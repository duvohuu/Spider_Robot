import React, { useMemo } from 'react';
import { Canvas, useThree } from '@react-three/fiber';
import { OrbitControls, Line } from '@react-three/drei';
import * as THREE from 'three';
import { useRobot } from '../../context/RobotContext';
import { LEGS, L1, L2, L3, STAND_X } from '../../constants/robotConfig';

// ── Palette ──────────────────────────────────────────────────────────────────
const SCENE_BG   = '#020b16';
const LEFT_COLOR = '#00e5ff';
const RIGHT_COLOR= '#ff2d78';
const LEFT_EMI   = '#004466';
const RIGHT_EMI  = '#660022';
const FOOT_COLOR = '#ffeb3b';
const BODY_EDGE  = '#ff2d78';
const BEACON     = '#00e5ff';

// ── Coordinate helper ─────────────────────────────────────────────────────────
// Robot frame: X/Y = horizontal plane, Z = vertical (positive = downward)
// three.js:    X/Z = horizontal ground plane, Y = up
// Mapping: robot (rx, ry, rz) → three.js (rx, -rz, -ry)
// −ry: mirror left/right so robot's left appears on viewer's right (facing convention)
const rw = (rx, ry, rz) => new THREE.Vector3(rx, -rz, -ry);

// ── Display constants ────────────────────────────────────────────────────────
const FLOOR_Y  = -9;   // three.js Y của mặt đất (cố định)

// Tính groupY để chân luôn chạm đất khi bodyHeight thay đổi
// Từ IK: totalAngle = 90° luôn → sin(totalAngle)=1
// footLocalZ (robot frame) ≈ femurLen*sin(femurRad) + tibiaLen
// three.js: foot_local_y = -footLocalZ
// groupY + foot_local_y = FLOOR_Y  →  groupY = FLOOR_Y + footLocalZ
const computeGroupY = (h) => {
    const horiz    = (STAND_X - L1) / LEG_S;
    const dz       = (h - L3) / LEG_S;
    const femurRad = Math.atan2(dz, horiz);
    const footLocalZ = (L2 / LEG_S) * Math.sin(femurRad) + (L3 / LEG_S);
    return FLOOR_Y + footLocalZ;
};

const BODY_R   = 10.0;
const LEG_S    = 15;

// ── Leg ──────────────────────────────────────────────────────────────────────
const Leg = React.memo(({ legId, mountDeg, leg }) => {
    if (!leg) return null;

    const toRad = (d) => (d * Math.PI) / 180;

    const mountRad = toRad(mountDeg);
    const coxaRad  = toRad(leg.coxa);
    const femurRad = toRad(leg.femur);
    const tibiaRad = toRad(leg.tibia);

    const bodyR   = BODY_R;
    const ox = bodyR * Math.cos(mountRad);
    const oy = bodyR * Math.sin(mountRad);
    const oz = 0;

    const coxaLen  = L1 / LEG_S;
    const femurLen = L2 / LEG_S;
    const tibiaLen = L3 / LEG_S;

    const cx1 = ox + coxaLen * Math.cos(mountRad + coxaRad);
    const cy1 = oy + coxaLen * Math.sin(mountRad + coxaRad);
    const cz1 = oz;

    const cx2 = cx1 + femurLen * Math.cos(mountRad + coxaRad) * Math.cos(femurRad);
    const cy2 = cy1 + femurLen * Math.sin(mountRad + coxaRad) * Math.cos(femurRad);
    const cz2 = cz1 + femurLen * Math.sin(femurRad);

    const totalAngle = femurRad + tibiaRad;
    const cx3 = cx2 + tibiaLen * Math.cos(mountRad + coxaRad) * Math.cos(totalAngle);
    const cy3 = cy2 + tibiaLen * Math.sin(mountRad + coxaRad) * Math.cos(totalAngle);
    const cz3 = cz2 + tibiaLen * Math.sin(totalAngle);

    // Apply coordinate mapping: robot (x,y,z_down) → three.js (x, -z_down, y)
    const points = [
        rw(ox,  oy,  oz),
        rw(cx1, cy1, cz1),
        rw(cx2, cy2, cz2),
        rw(cx3, cy3, cz3),
    ];

    const isLeft  = legId.startsWith('L');
    const baseCol = leg.valid ? (isLeft ? LEFT_COLOR  : RIGHT_COLOR) : '#2a2a2a';
    const emiCol  = leg.valid ? (isLeft ? LEFT_EMI    : RIGHT_EMI)   : '#111';

    // Tube radii: coxa → femur → tibia (tapering)
    const tubeR   = [0.13, 0.10, 0.07];

    return (
        <group>
            {/* Bone segments */}
            {points.slice(0, -1).map((p, i) => {
                const q    = points[i + 1];
                const mid  = new THREE.Vector3().addVectors(p, q).multiplyScalar(0.5);
                const dir  = new THREE.Vector3().subVectors(q, p);
                const len  = dir.length();
                const quat = new THREE.Quaternion().setFromUnitVectors(
                    new THREE.Vector3(0, 1, 0),
                    dir.clone().normalize(),
                );
                return (
                    <mesh key={i} position={mid} quaternion={quat}>
                        <cylinderGeometry args={[tubeR[i], tubeR[i], len, 12]} />
                        <meshStandardMaterial
                            color={baseCol}
                            emissive={emiCol}
                            emissiveIntensity={1.2}
                            metalness={0.4}
                            roughness={0.35}
                        />
                    </mesh>
                );
            })}

            {/* Joints */}
            {points.map((p, i) => {
                const isFoot = i === points.length - 1;
                const r   = isFoot ? 0.22 : i === 0 ? 0.08 : 0.14;
                const col = isFoot ? FOOT_COLOR : baseCol;
                const emi = isFoot ? '#806000'  : emiCol;
                const eiI = isFoot ? 2.0 : 1.2;
                return (
                    <mesh key={`j${i}`} position={p}>
                        <sphereGeometry args={[r, 14, 14]} />
                        <meshStandardMaterial
                            color={col}
                            emissive={emi}
                            emissiveIntensity={eiI}
                        />
                    </mesh>
                );
            })}
        </group>
    );
}, (prev, next) =>
    prev.leg?.coxa  === next.leg?.coxa  &&
    prev.leg?.femur === next.leg?.femur &&
    prev.leg?.tibia === next.leg?.tibia &&
    prev.leg?.valid === next.leg?.valid
);

// ── Body ─────────────────────────────────────────────────────────────────────
const Body = () => {
    const r  = BODY_R;
    const r2 = r * 0.52;

    const shape = useMemo(() => {
        const s = new THREE.Shape();
        for (let i = 0; i < 6; i++) {
            const a = (i / 6) * Math.PI * 2;
            i === 0
                ? s.moveTo(r * Math.cos(a), r * Math.sin(a))
                : s.lineTo(r * Math.cos(a), r * Math.sin(a));
        }
        s.closePath();
        return s;
    }, [r]);

    // Hex ring points in three.js XZ ground plane
    // Z negated to stay consistent with rw() −ry mirror
    const outerPts = useMemo(() => {
        const pts = [];
        for (let i = 0; i <= 6; i++) {
            const a = (i / 6) * Math.PI * 2;
            pts.push([r * Math.cos(a), 0.15, -r * Math.sin(a)]);
        }
        return pts;
    }, [r]);

    const innerPts = useMemo(() => {
        const pts = [];
        for (let i = 0; i <= 6; i++) {
            const a = (i / 6) * Math.PI * 2;
            pts.push([r2 * Math.cos(a), 0.15, -r2 * Math.sin(a)]);
        }
        return pts;
    }, [r2]);

    // Spoke points (center to outer vertex pairs)
    const spokePairs = useMemo(() => {
        const pairs = [];
        for (let i = 0; i < 6; i++) {
            const a = (i / 6) * Math.PI * 2;
            pairs.push([[0, 0.15, 0], [r2 * Math.cos(a), 0.15, -r2 * Math.sin(a)]]);
        }
        return pairs;
    }, [r2]);

    return (
        <group>
            {/* Solid body */}
            <mesh rotation={[Math.PI / 2, 0, 0]}>
                <extrudeGeometry args={[shape, { depth: 0.3, bevelEnabled: false }]} />
                <meshStandardMaterial color="#040e1c" metalness={0.8} roughness={0.25} />
            </mesh>

            {/* Outer hex ring */}
            <Line points={outerPts} color={BODY_EDGE} lineWidth={2.0} />

            {/* Inner hex ring */}
            <Line points={innerPts} color={BODY_EDGE} lineWidth={1.0} opacity={0.5} transparent />

            {/* Spokes */}
            {spokePairs.map((pair, i) => (
                <Line key={i} points={pair} color={BODY_EDGE} lineWidth={0.8} opacity={0.3} transparent />
            ))}

            {/* Center beacon */}
            <mesh position={[0, 0.3, 0]}>
                <sphereGeometry args={[0.22, 16, 16]} />
                <meshStandardMaterial color={BEACON} emissive={BEACON} emissiveIntensity={4.0} />
            </mesh>

            {/* Beacon glow halo */}
            <mesh position={[0, 0.3, 0]}>
                <sphereGeometry args={[0.40, 10, 10]} />
                <meshStandardMaterial color={BEACON} emissive={BEACON} emissiveIntensity={1.0} opacity={0.12} transparent />
            </mesh>
        </group>
    );
};

// ── Floor grid (canvas texture — visible from any angle) ─────────────────────
const FloorGrid = ({ y = -9 }) => {
    const { gl } = useThree();

    const texture = useMemo(() => {
        const SIZE   = 1024;
        const CELLS  = 20;          // số ô nhỏ trên 1 cạnh
        const canvas = document.createElement('canvas');
        canvas.width = canvas.height = SIZE;
        const ctx = canvas.getContext('2d');

        // Nền
        ctx.fillStyle = '#030c18';
        ctx.fillRect(0, 0, SIZE, SIZE);

        const cell = SIZE / CELLS;

        // Đường ô nhỏ
        ctx.strokeStyle = '#0d2a45';
        ctx.lineWidth   = 1.5;
        for (let i = 0; i <= CELLS; i++) {
            const p = i * cell;
            ctx.beginPath(); ctx.moveTo(p, 0); ctx.lineTo(p, SIZE); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(0, p); ctx.lineTo(SIZE, p); ctx.stroke();
        }

        // Đường ô lớn (mỗi 5 ô nhỏ)
        ctx.strokeStyle = '#0088cc';
        ctx.lineWidth   = 3.5;
        for (let i = 0; i <= CELLS; i += 5) {
            const p = i * cell;
            ctx.beginPath(); ctx.moveTo(p, 0); ctx.lineTo(p, SIZE); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(0, p); ctx.lineTo(SIZE, p); ctx.stroke();
        }

        const tex = new THREE.CanvasTexture(canvas);
        tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
        tex.repeat.set(5, 5);   // lặp 5×5 trên plane 100×100
        tex.anisotropy = gl.capabilities.getMaxAnisotropy(); // giữ nét ở góc nghiêng
        tex.needsUpdate = true;
        return tex;
    }, [gl]);

    return (
        <mesh position={[0, y, 0]} rotation={[-Math.PI / 2, 0, 0]}>
            <planeGeometry args={[100, 100]} />
            <meshBasicMaterial map={texture} />
        </mesh>
    );
};

// ── Scene ─────────────────────────────────────────────────────────────────────
const SceneContent = () => {
    const { angles, bodyHeight } = useRobot();
    const groupY = computeGroupY(bodyHeight);
    return (
    <>
        <ambientLight intensity={0.18} color="#061220" />
        <directionalLight position={[6, 14, 8]} intensity={0.7} castShadow />
        <pointLight position={[0,  8,  0]} intensity={2.5} color="#00b4d8" distance={28} />
        <pointLight position={[6,  4, -5]} intensity={1.2} color="#ff2d78" distance={20} />
        <pointLight position={[-6, 4,  5]} intensity={0.8} color="#00e5ff" distance={18} />

        {/* Robot group — di chuyển lên/xuống theo bodyHeight, chân luôn chạm đất */}
        <group position={[0, groupY, 0]}>
            <Body />
            {LEGS.map((leg) => (
                <Leg key={leg.id} legId={leg.id} mountDeg={leg.mountDeg} leg={angles[leg.id]} />
            ))}
        </group>

        {/* Floor cố định */}
        <FloorGrid y={FLOOR_Y} />

        <OrbitControls
            makeDefault
            target={[0, -4, 0]}
            enablePan
            enableZoom
            enableRotate
        />
    </>
    );
};

// ── Main export ───────────────────────────────────────────────────────────────
const SpiderViewer = () => (
    <Canvas
        camera={{ position: [9, 13, 16], fov: 42 }}
        shadows
        style={{ background: SCENE_BG, width: '100%', height: '100%' }}
    >
        <SceneContent />
    </Canvas>
);

export default SpiderViewer;

