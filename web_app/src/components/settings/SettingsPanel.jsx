import {
    Box, Typography, Slider, Stack, Divider, Accordion, Tab, Tabs,
    AccordionSummary, AccordionDetails, IconButton, Tooltip,
} from '@mui/material';
import DirectionsRunIcon from '@mui/icons-material/DirectionsRun';
import OpenWithIcon from '@mui/icons-material/OpenWith';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import { useRobot } from '../../context/RobotContext';
import { LEGS } from '../../constants/robotConfig';

const CYAN = '#00e5ff';
const PINK = '#ff2d78';

// ==================== SUB-COMPONENTS ====================

const SectionLabel = ({ children }) => (
    <Typography
        sx={{
            fontSize: 11,
            fontWeight: 700,
            letterSpacing: '0.14em',
            textTransform: 'uppercase',
            color: '#4a8090',
            mb: 0.75,
        }}
    >
        {children}
    </Typography>
);

const SliderRow = ({ label, value, min, max, step = 0.5, onChange }) => {
    return (
        <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 0.6 }}>
            <Typography sx={{ fontSize: 13, minWidth: 46, color: '#6a90a0', fontWeight: 500 }}>
                {label}
            </Typography>
            <Slider
                size="small"
                value={value}
                min={min}
                max={max}
                step={step}
                onChange={(_, v) => onChange(v)}
                sx={{
                    flex: 1,
                    color: CYAN,
                    '& .MuiSlider-thumb': { boxShadow: `0 0 8px ${CYAN}60`, width: 14, height: 14 },
                    '& .MuiSlider-rail': { opacity: 0.25 },
                }}
            />
            <Typography sx={{ fontSize: 13, minWidth: 44, textAlign: 'right', color: CYAN, fontFamily: 'monospace', fontWeight: 700 }}>
                {Number(value).toFixed(1)}
            </Typography>
        </Stack>
    );
};

const LegOffsetPanel = ({ legId }) => {
    const { offsets, updateOffset } = useRobot();
    const off    = offsets[legId];
    const isLeft = legId.startsWith('L');
    const color  = isLeft ? CYAN : PINK;

    return (
        <Accordion
            disableGutters
            sx={{
                mb: 0.4,
                border: `1px solid ${color}18`,
                '&:before': { display: 'none' },
            }}
        >
            <AccordionSummary
                expandIcon={<ExpandMoreIcon sx={{ fontSize: 18, color: '#3a6070' }} />}
                sx={{ minHeight: 38, py: 0, px: 1.25 }}
            >
                <Stack direction="row" alignItems="center" spacing={0.75}>
                    <Box
                        sx={{
                            width: 7, height: 7, borderRadius: '50%',
                            bgcolor: color, boxShadow: `0 0 6px ${color}`,
                        }}
                    />
                    <Typography sx={{ fontSize: 13, fontWeight: 700, color, fontFamily: 'monospace' }}>
                        {legId}
                    </Typography>
                    <Typography sx={{ fontSize: 12, color: '#3a6070' }}>Offset</Typography>
                </Stack>
            </AccordionSummary>
            <AccordionDetails sx={{ pt: 0.5, pb: 1, px: 1 }}>
                <SliderRow label="Coxa"  value={off.cx} min={-30} max={30} step={0.5} onChange={(v) => updateOffset(legId, 'cx', v)} />
                <SliderRow label="Femur" value={off.fm} min={-30} max={30} step={0.5} onChange={(v) => updateOffset(legId, 'fm', v)} />
                <SliderRow label="Tibia" value={off.tb} min={-30} max={30} step={0.5} onChange={(v) => updateOffset(legId, 'tb', v)} />
            </AccordionDetails>
        </Accordion>
    );
};

const GaitPanel = () => {
    const { gait, updateGait } = useRobot();
    return (
        <Box>
            <SliderRow label="Step X"   value={gait.stepX}    min={5}   max={60}  step={1}  onChange={(v) => updateGait('stepX', v)} />
            <SliderRow label="Lift Z"   value={gait.liftZ}    min={10}  max={80}  step={1}  onChange={(v) => updateGait('liftZ', v)} />
            <SliderRow label="T Lift"   value={gait.tLiftMs}  min={40}  max={400} step={5}  onChange={(v) => updateGait('tLiftMs', v)} />
            <SliderRow label="T Swing"  value={gait.tSwingMs} min={80}  max={600} step={5}  onChange={(v) => updateGait('tSwingMs', v)} />
            <SliderRow label="T Down"   value={gait.tDownMs}  min={40}  max={400} step={5}  onChange={(v) => updateGait('tDownMs', v)} />
            <SliderRow label="Trim Y"   value={gait.trimY}    min={-30} max={30}  step={0.5} onChange={(v) => updateGait('trimY', v)} />
        </Box>
    );
};

const FootPositionPanel = () => {
    const { footPositions, updateFootPosition } = useRobot();
    return (
        <Box>
            {LEGS.map(({ id }) => {
                const isLeft = id.startsWith('L');
                const color  = isLeft ? CYAN : PINK;
                return (
                    <Accordion
                        key={id}
                        disableGutters
                        sx={{
                            mb: 0.4,
                            border: `1px solid ${color}18`,
                            '&:before': { display: 'none' },
                        }}
                    >
                        <AccordionSummary
                            expandIcon={<ExpandMoreIcon sx={{ fontSize: 18, color: '#3a6070' }} />}
                            sx={{ minHeight: 38, py: 0, px: 1.25 }}
                        >
                            <Stack direction="row" alignItems="center" spacing={0.75}>
                                <Box
                                    sx={{
                                        width: 7, height: 7, borderRadius: '50%',
                                        bgcolor: color, boxShadow: `0 0 6px ${color}`,
                                    }}
                                />
                                <Typography sx={{ fontSize: 13, fontWeight: 700, color, fontFamily: 'monospace' }}>
                                    {id}
                                </Typography>
                                <Typography sx={{ fontSize: 12, color: '#3a6070' }}>XYZ</Typography>
                            </Stack>
                        </AccordionSummary>
                        <AccordionDetails sx={{ pt: 0.5, pb: 1, px: 1 }}>
                            {['x', 'y', 'z'].map((axis) => (
                                <SliderRow
                                    key={axis}
                                    label={axis.toUpperCase()}
                                    value={footPositions[id][axis]}
                                    min={-150}
                                    max={200}
                                    step={1}
                                    onChange={(v) => updateFootPosition(id, axis, v)}
                                />
                            ))}
                        </AccordionDetails>
                    </Accordion>
                );
            })}
        </Box>
    );
};

// ==================== MAIN COMPONENT ====================

const tabSx = (active, color) => ({
    minHeight: 34,
    py: 0.5,
    fontSize: 11,
    fontWeight: 700,
    letterSpacing: '0.08em',
    color: active ? color : '#3a6070',
    textTransform: 'uppercase',
    minWidth: 0,
    flex: 1,
});

const SettingsPanel = () => {
    const { resetToStand, bodyHeight, setBodyHeight, controlMode, setControlMode } = useRobot();

    return (
        <Box sx={{ height: '100%', overflow: 'auto', px: 1.25, py: 1.5 }}>

            {/* Header */}
            <Stack direction="row" alignItems="center" justifyContent="space-between" sx={{ mb: 1 }}>
                <Stack direction="row" alignItems="center" spacing={1}>
                    <Box sx={{ width: 3, height: 18, bgcolor: CYAN, borderRadius: 1 }} />
                    <Typography sx={{ fontSize: 13, fontWeight: 700, letterSpacing: '0.1em', textTransform: 'uppercase', color: '#d0eaf8' }}>
                        Controls
                    </Typography>
                </Stack>
                <Tooltip title="Reset to stand pose">
                    <IconButton
                        size="small"
                        onClick={resetToStand}
                        sx={{
                            color: '#ffab40',
                            border: '1px solid #ffab4030',
                            borderRadius: 1.5,
                            p: 0.5,
                            '&:hover': { bgcolor: '#ffab4015' },
                        }}
                    >
                        <RestartAltIcon sx={{ fontSize: 16 }} />
                    </IconButton>
                </Tooltip>
            </Stack>

            {/* Mode tabs */}
            <Tabs
                value={controlMode}
                onChange={(_, v) => setControlMode(v)}
                sx={{
                    mb: 1.25,
                    minHeight: 34,
                    bgcolor: '#030c18',
                    borderRadius: 1.5,
                    border: '1px solid #0d2030',
                    '& .MuiTabs-indicator': {
                        height: 2,
                        borderRadius: 1,
                        bgcolor: controlMode === 'gait' ? '#00e676' : PINK,
                    },
                }}
            >
                <Tab
                    value="gait"
                    icon={<DirectionsRunIcon sx={{ fontSize: 14 }} />}
                    iconPosition="start"
                    label="Gait"
                    sx={tabSx(controlMode === 'gait', '#00e676')}
                />
                <Tab
                    value="pose"
                    icon={<OpenWithIcon sx={{ fontSize: 14 }} />}
                    iconPosition="start"
                    label="Static Pose"
                    sx={tabSx(controlMode === 'pose', PINK)}
                />
            </Tabs>

            {/* ── GAIT MODE ── */}
            {controlMode === 'gait' && (<>
                {/* Body Height */}
                <SectionLabel>Body Height</SectionLabel>
                <SliderRow label="Z" value={bodyHeight} min={60} max={180} step={1} onChange={setBodyHeight} />

                <Divider sx={{ my: 1.25, borderColor: '#0d2030' }} />

                {/* Gait */}
                <SectionLabel>Gait Parameters</SectionLabel>
                <GaitPanel />

                <Divider sx={{ my: 1.25, borderColor: '#0d2030' }} />

                {/* Servo Offsets */}
                <SectionLabel>Servo Offsets</SectionLabel>
                {LEGS.map(({ id }) => <LegOffsetPanel key={id} legId={id} />)}
            </>)}

            {/* ── POSE MODE ── */}
            {controlMode === 'pose' && (<>
                <Box sx={{
                    mb: 1.25, px: 1, py: 0.75,
                    bgcolor: `${PINK}10`, border: `1px solid ${PINK}30`,
                    borderRadius: 1.5,
                }}>
                    <Typography sx={{ fontSize: 11, color: PINK, lineHeight: 1.4 }}>
                        Điều khiển từng chân riêng lẻ.<br />
                        Dừng gait trước khi dùng mode này.
                    </Typography>
                </Box>

                <SectionLabel>Foot Positions (Body Frame)</SectionLabel>
                <FootPositionPanel />

                <Divider sx={{ my: 1.25, borderColor: '#0d2030' }} />

            </>)}
        </Box>
    );
};

export default SettingsPanel;