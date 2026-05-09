import React, { createContext, useContext, useState, useEffect, useMemo, useRef } from 'react';
import {
    LEGS,
    DEFAULT_OFFSETS,
    DEFAULT_GAIT,
    STAND_X,
    STAND_Z,
} from '../constants/robotConfig';
import { inverseKinematics, bodyToLegFrame } from '../utils/kinematics';
import { useWs } from './WsContext';

// ==================== HELPERS ====================

const computeStandPose = () => {
    const positions = {};
    LEGS.forEach(({ id, mountDeg }) => {
        const sin = Math.sin((mountDeg * Math.PI) / 180);
        const cos = Math.cos((mountDeg * Math.PI) / 180);
        positions[id] = { x: STAND_X * cos, y: STAND_X * sin, z: STAND_Z };
    });
    return positions;
};

const computeAngles = (footPositions, offsets) => {
    const angles = {};
    LEGS.forEach(({ id, mountDeg }) => {
        const p = footPositions[id];
        const { lx, ly } = bodyToLegFrame(p.x, p.y, mountDeg);
        const ik = inverseKinematics(lx, ly, p.z);
        angles[id] = {
            coxa:  ik.coxaDeg  + (offsets[id]?.cx ?? 0),
            femur: ik.femurDeg + (offsets[id]?.fm ?? 0),
            tibia: ik.tibiaDeg + (offsets[id]?.tb ?? 0),
            // Góc hiển thị viewer (không có offset để FK chính xác)
            viewCoxa:  ik.coxaDeg,
            viewFemur: ik.femurDeg,
            viewTibia: ik.tibiaDeg,
            valid: ik.valid,
        };
    });
    return angles;
};

// ── Gait animation constants (mirrors firmware) ──────────────
const STAND_POSE_BASE = computeStandPose(); // fixed reference base positions
const GAIT_STEP_X  = 15;    // mm — same as firmware STEP_X
const GAIT_LIFT_Z  = 30;    // mm — same as firmware LIFT_Z
const GAIT_PERIOD  = 1200;  // ms — full cycle
const SWING_FRAC   = 0.38;  // fraction of cycle for swing phase
// Tripod groups: A={LF,RM,LB} phase=0.0,  B={RF,LM,RB} phase=0.5
const LEG_PHASE = { LF: 0.0, RM: 0.0, LB: 0.0, RF: 0.5, LM: 0.5, RB: 0.5 };

// ==================== CONTEXT ====================

const RobotContext = createContext(null);

const RobotProvider = ({ children }) => {
    const { feetData, send, connected } = useWs();
    const [offsets, setOffsets] = useState(DEFAULT_OFFSETS);
    const [gait, setGait] = useState(DEFAULT_GAIT);
    const [footPositions, setFootPositions] = useState(computeStandPose);
    const [bodyHeight, setBodyHeight] = useState(STAND_Z);
    const [animating, setAnimating] = useState(false);

    // Sync bodyHeight vào ref để RAF closure dùng được mà không stale
    const bodyHeightRef = useRef(STAND_Z);
    useEffect(() => { bodyHeightRef.current = bodyHeight; }, [bodyHeight]);

    // Mode: 'gait' | 'pose'
    const [controlMode, setControlModeState] = useState('gait');
    const controlModeRef = useRef('gait');
    const setControlMode = (mode) => {
        setControlModeState(mode);
        controlModeRef.current = mode;
        if (mode === 'pose') {
            // Sync foot z với bodyHeight hiện tại
            setFootPositions(prev => {
                const updated = {};
                LEGS.forEach(({ id }) => { updated[id] = { ...prev[id], z: bodyHeight }; });
                return updated;
            });
        } else {
            // Quay lại gait: reset về stand pose
            setFootPositions(computeStandPose());
        }
    };

    // Cập nhật viewer từ dữ liệu thật của robot
    useEffect(() => {
        if (!feetData) return;
        setFootPositions({
            LF: feetData.LF,
            LM: feetData.LM,
            LB: feetData.LB,
            RF: feetData.RF,
            RM: feetData.RM,
            RB: feetData.RB,
        });
    }, [feetData]);

    // Gait animation: RAF loop mô phỏng tripod gait khi animating=true
    useEffect(() => {
        if (!animating) {
            setFootPositions(computeStandPose());
            return;
        }
        let rafId;
        const startTime = performance.now();
        const animate = () => {
            const t = ((performance.now() - startTime) % GAIT_PERIOD) / GAIT_PERIOD;
            const h = bodyHeightRef.current;
            const np = {};
            LEGS.forEach(({ id }) => {
                const base = STAND_POSE_BASE[id];
                const ph   = (t + LEG_PHASE[id]) % 1.0;
                let dx = 0, dz = 0;
                if (ph < SWING_FRAC) {
                    // Swing: nhấc chân và đưa ra trước
                    const s = ph / SWING_FRAC;
                    dx = GAIT_STEP_X * (2 * s - 1);
                    dz = -GAIT_LIFT_Z * Math.sin(s * Math.PI);
                } else {
                    // Stance: đẩy chân về sau (thân tiến lên)
                    const s = (ph - SWING_FRAC) / (1 - SWING_FRAC);
                    dx = GAIT_STEP_X * (1 - 2 * s);
                }
                np[id] = { x: base.x + dx, y: base.y, z: h + dz };
            });
            setFootPositions(np);
            rafId = requestAnimationFrame(animate);
        };
        rafId = requestAnimationFrame(animate);
        return () => {
            cancelAnimationFrame(rafId);
            setFootPositions(computeStandPose());
        };
    }, [animating]); // bodyHeight đọc qua ref, không cần dep

    // Vị trí hiệu dụng: pose mode dùng footPositions trực tiếp; gait mode merge bodyHeight vào z
    const effectivePositions = useMemo(() => {
        if (feetData) return footPositions;
        if (controlModeRef.current === 'pose') return footPositions;
        if (animating) return footPositions; // z đang được animation RAF set, không override
        const updated = {};
        LEGS.forEach(({ id }) => {
            updated[id] = { ...footPositions[id], z: bodyHeight };
        });
        return updated;
    }, [footPositions, bodyHeight, feetData, controlMode, animating]);

    // Góc khớp: chỉ tính lại khi effectivePositions hoặc offsets thực sự thay đổi
    const angles = useMemo(() => computeAngles(effectivePositions, offsets), [effectivePositions, offsets]);

    // Gửi set_height khi bodyHeight thay đổi (debounce 80ms) HOẶC khi vừa kết nối
    useEffect(() => {
        if (!connected) return;
        const t = setTimeout(() => {
            send({ cmd: 'set_height', z: bodyHeight });
        }, 80);
        return () => clearTimeout(t);
    }, [bodyHeight, connected, send]);

    // Gửi toàn bộ offset khi vừa kết nối để sync firmware
    useEffect(() => {
        if (!connected) return;
        LEGS.forEach(({ id }) => {
            const o = offsets[id];
            send({ cmd: 'set_offset', leg: id, cx: o.cx, fm: o.fm, tb: o.tb });
        });
        // Sync gait params
        send({ cmd: 'set_gait', ...gait });
    }, [connected]); // chỉ khi connected thay đổi

    const updateOffset = (legId, joint, value) => {
        setOffsets((prev) => {
            const newOff = { ...prev, [legId]: { ...prev[legId], [joint]: value } };
            if (connected) {
                const o = newOff[legId];
                send({ cmd: 'set_offset', leg: legId, cx: o.cx, fm: o.fm, tb: o.tb });
            }
            return newOff;
        });
    };

    const updateGait = (key, value) => {
        setGait((prev) => {
            const newGait = { ...prev, [key]: value };
            if (connected) {
                send({ cmd: 'set_gait', ...newGait });
            }
            return newGait;
        });
    };

    const updateFootPosition = (legId, axis, value) => {
        setFootPositions((prev) => {
            const newPos = { ...prev, [legId]: { ...prev[legId], [axis]: value } };
            // Pose mode: gửi set_foot ngay khi slider thay đổi
            if (controlModeRef.current === 'pose' && connected) {
                const p = newPos[legId];
                send({ cmd: 'set_foot', leg: legId, x: p.x, y: p.y, z: p.z });
            }
            return newPos;
        });
    };

    const resetToStand = () => {
        setFootPositions(computeStandPose());
        setBodyHeight(STAND_Z);
    };

    return (
        <RobotContext.Provider value={{
            offsets, updateOffset,
            gait, updateGait,
            footPositions: effectivePositions, updateFootPosition,
            bodyHeight, setBodyHeight,
            angles,
            animating, setAnimating,
            controlMode, setControlMode,
            resetToStand,
        }}>
            {children}
        </RobotContext.Provider>
    );
};

const useRobot = () => useContext(RobotContext);

export { RobotProvider, useRobot };
