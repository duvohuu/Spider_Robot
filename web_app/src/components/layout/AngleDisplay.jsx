import React from 'react';
import { Box, Stack, Typography, Divider } from '@mui/material';
import { useRobot } from '../../context/RobotContext';
import { LEGS } from '../../constants/robotConfig';

const CYAN  = '#00e5ff';
const PINK  = '#ff2d78';
const ERR   = '#ff2d78';

const AngleCell = ({ label, value, valid }) => (
    <Stack alignItems="center" sx={{ flex: 1, gap: 0.15 }}>
        <Typography sx={{ fontSize: 10, color: '#3a6070', letterSpacing: '0.06em', lineHeight: 1, textTransform: 'uppercase' }}>
            {label}
        </Typography>
        <Typography
            sx={{
                fontSize: 13,
                fontFamily: 'monospace',
                fontWeight: 700,
                color: valid ? '#c8e8f8' : ERR,
                lineHeight: 1.2,
                textShadow: valid ? '0 0 8px #00e5ff30' : '0 0 8px #ff2d7860',
            }}
        >
            {valid ? `${value.toFixed(1)}°` : 'ERR'}
        </Typography>
    </Stack>
);

const AngleDisplay = () => {
    const { angles } = useRobot();

    return (
        <Box sx={{ p: 1.5 }}>
            <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1.25 }}>
                <Box sx={{ width: 3, height: 14, bgcolor: CYAN, borderRadius: 1, boxShadow: `0 0 6px ${CYAN}` }} />
                <Typography
                    sx={{
                        fontSize: 11,
                        fontWeight: 800,
                        letterSpacing: '0.14em',
                        textTransform: 'uppercase',
                        color: CYAN,
                        textShadow: `0 0 10px ${CYAN}60`,
                    }}
                >
                    Joint Angles
                </Typography>
            </Stack>

            <Stack spacing={0.6}>
                {LEGS.map(({ id }) => {
                    const a      = angles[id];
                    const isLeft = id.startsWith('L');
                    const accentColor = isLeft ? CYAN : PINK;

                    return (
                        <Box
                            key={id}
                            sx={{
                                bgcolor: '#040f1c',
                                border: `1px solid ${accentColor}25`,
                                borderLeft: `3px solid ${accentColor}`,
                                borderRadius: '0 6px 6px 0',
                                px: 1.25,
                                py: 0.75,
                                boxShadow: `inset 0 0 12px ${accentColor}08`,
                                transition: 'box-shadow 0.2s',
                                '&:hover': { boxShadow: `inset 0 0 16px ${accentColor}15` },
                            }}
                        >
                            <Stack direction="row" alignItems="center" spacing={1}>
                                <Typography
                                    sx={{
                                        fontSize: 12,
                                        fontWeight: 800,
                                        fontFamily: 'monospace',
                                        color: accentColor,
                                        minWidth: 28,
                                        letterSpacing: '0.04em',
                                        textShadow: `0 0 8px ${accentColor}60`,
                                    }}
                                >
                                    {id}
                                </Typography>
                                <Divider orientation="vertical" flexItem sx={{ borderColor: '#0d2030' }} />
                                <AngleCell label="Cx" value={a?.coxa  ?? 0} valid={a?.valid} />
                                <AngleCell label="Fm" value={a?.femur ?? 0} valid={a?.valid} />
                                <AngleCell label="Tb" value={a?.tibia ?? 0} valid={a?.valid} />
                            </Stack>
                        </Box>
                    );
                })}
            </Stack>
        </Box>
    );
};

export default AngleDisplay;
