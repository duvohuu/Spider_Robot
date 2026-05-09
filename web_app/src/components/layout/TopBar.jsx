import React from 'react';
import {
    AppBar, Toolbar, Typography, Chip, TextField,
    Button, Tooltip, Box, IconButton, Stack,
} from '@mui/material';
import WifiIcon from '@mui/icons-material/Wifi';
import WifiOffIcon from '@mui/icons-material/WifiOff';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import StopIcon from '@mui/icons-material/Stop';
import { useWs } from '../../context/WsContext';
import { useRobot } from '../../context/RobotContext';

const TopBar = () => {
    const { connected, robotState, latency, url, setUrl, connect, disconnect, send } = useWs();
    const { animating, setAnimating, gait, controlMode } = useRobot();

    // ==================== HANDLERS ====================

    const handleStart = () => {
        send({ cmd: 'start', gait });
        setAnimating(true);
    };

    const handleStop = () => {
        send({ cmd: 'stop' });
        setAnimating(false);
    };

    // ==================== RENDER ====================

    return (
        <AppBar
            position="static"
            elevation={0}
            sx={{
                bgcolor: '#020e1a',
                borderBottom: '1px solid #0d2a3e',
                backgroundImage: 'none',
                boxShadow: '0 2px 16px #000a',
            }}
        >
            <Toolbar variant="dense" sx={{ gap: 1.5, minHeight: 58, overflow: 'hidden', px: 2 }}>

                {/* Logo */}
                <Stack direction="row" alignItems="center" spacing={1} sx={{ mr: 1.5, flexShrink: 0 }}>
                    <Box sx={{ position: 'relative', width: 14, height: 14 }}>
                        <Box sx={{
                            width: 10, height: 10, borderRadius: '50%',
                            bgcolor: '#00e5ff', boxShadow: '0 0 12px #00e5ff, 0 0 24px #00e5ff60',
                            position: 'absolute', top: 2, left: 2,
                        }} />
                        <Box sx={{
                            width: 14, height: 14, borderRadius: '50%',
                            border: '1px solid #00e5ff40',
                            position: 'absolute', top: 0, left: 0,
                            animation: 'pulse 2s ease-in-out infinite',
                            '@keyframes pulse': {
                                '0%, 100%': { opacity: 0.4, transform: 'scale(1)' },
                                '50%': { opacity: 1, transform: 'scale(1.2)' },
                            },
                        }} />
                    </Box>
                    <Typography
                        variant="subtitle1"
                        fontWeight={800}
                        sx={{
                            color: '#e0f4ff',
                            letterSpacing: '0.14em',
                            textTransform: 'uppercase',
                            fontSize: 15,
                            whiteSpace: 'nowrap',
                            textShadow: '0 0 12px #00e5ff60',
                        }}
                    >
                        Spider Robot
                    </Typography>
                    <Box sx={{
                        px: 0.8, py: 0.1,
                        border: '1px solid #00e5ff30',
                        borderRadius: 0.5,
                        bgcolor: '#00e5ff10',
                    }}>
                        <Typography sx={{ fontSize: 9, color: '#00e5ff90', letterSpacing: '0.1em', fontFamily: 'monospace' }}>v1.0</Typography>
                    </Box>
                </Stack>

                {/* Divider */}
                <Box sx={{ width: '1px', height: 24, bgcolor: '#0d2a3e', flexShrink: 0 }} />

                {/* WebSocket URL */}
                <TextField
                    size="small"
                    variant="outlined"
                    value={url}
                    onChange={(e) => setUrl(e.target.value)}
                    placeholder="ws://192.168.4.1/ws"
                    sx={{
                        width: 220,
                        flexShrink: 1,
                        '& .MuiOutlinedInput-root': {
                            fontSize: 13,
                            color: '#9ab8cc',
                            bgcolor: '#050f1c',
                            borderRadius: 1.5,
                            '& fieldset': { borderColor: '#0d2a3e' },
                            '&:hover fieldset': { borderColor: '#00e5ff55' },
                            '&.Mui-focused fieldset': { borderColor: '#00e5ff88', boxShadow: '0 0 8px #00e5ff20' },
                        },
                    }}
                    inputProps={{ spellCheck: false }}
                />

                {/* Connect/Disconnect */}
                <Button
                    size="small"
                    variant={connected ? 'outlined' : 'contained'}
                    color={connected ? 'error' : 'primary'}
                    onClick={connected ? disconnect : () => connect(url)}
                    startIcon={connected ? <WifiOffIcon fontSize="small" /> : <WifiIcon fontSize="small" />}
                    sx={{ minWidth: 110, fontSize: 13, py: 0.65, borderRadius: 1.5, flexShrink: 0, fontWeight: 700 }}
                >
                    {connected ? 'Disconnect' : 'Connect'}
                </Button>

                {/* Connection status */}
                <Chip
                    label={connected ? 'ONLINE' : 'OFFLINE'}
                    size="small"
                    sx={{
                        flexShrink: 0,
                        fontWeight: 800,
                        fontSize: 11,
                        letterSpacing: '0.1em',
                        height: 24,
                        bgcolor: connected ? '#00331860' : '#1a1a1a',
                        color: connected ? '#00e676' : '#506070',
                        border: `1px solid ${connected ? '#00e67660' : '#30404060'}`,
                        boxShadow: connected ? '0 0 8px #00e67630' : 'none',
                    }}
                />

                {/* Latency */}
                {connected && latency !== null && (
                    <Box sx={{ px: 1, py: 0.3, bgcolor: '#05141f', border: '1px solid #0d2a3e', borderRadius: 1, flexShrink: 0 }}>
                        <Typography sx={{ color: '#00e5ff', fontFamily: 'monospace', fontSize: 12, whiteSpace: 'nowrap' }}>
                            {latency}<Typography component="span" sx={{ fontSize: 10, color: '#3a6070', ml: 0.4 }}>ms</Typography>
                        </Typography>
                    </Box>
                )}

                {/* Robot state */}
                {connected && (
                    <Chip
                        label={robotState.toUpperCase()}
                        size="small"
                        variant="outlined"
                        sx={{
                            flexShrink: 0,
                            fontSize: 11,
                            fontWeight: 700,
                            letterSpacing: '0.08em',
                            height: 24,
                            color: robotState === 'running' ? '#ffab40' : '#5080a0',
                            borderColor: robotState === 'running' ? '#ffab4060' : '#30404060',
                            boxShadow: robotState === 'running' ? '0 0 8px #ffab4040' : 'none',
                        }}
                    />
                )}

                <Box sx={{ flex: 1 }} />

                {/* Motion controls */}
                <Tooltip title={controlMode === 'pose' ? 'Switch to Gait mode first' : 'Start tripod gait'}>
                    <span>
                        <IconButton
                            size="small"
                            disabled={!connected || animating || controlMode === 'pose'}
                            onClick={handleStart}
                            sx={{
                                flexShrink: 0,
                                color: (!connected || animating || controlMode === 'pose') ? '#304050' : '#00e676',
                                border: '1px solid',
                                borderColor: (!connected || animating || controlMode === 'pose') ? '#1a2a34' : '#00e67650',
                                borderRadius: 1.5,
                                p: 1,
                                bgcolor: (!connected || animating || controlMode === 'pose') ? 'transparent' : '#00e67610',
                                boxShadow: (!connected || animating || controlMode === 'pose') ? 'none' : '0 0 10px #00e67630',
                                '&:hover': { bgcolor: '#00e67620' },
                            }}
                        >
                            <PlayArrowIcon />
                        </IconButton>
                    </span>
                </Tooltip>

                <Tooltip title="Stop">
                    <span>
                        <IconButton
                            size="small"
                            disabled={!animating}
                            onClick={handleStop}
                            sx={{
                                flexShrink: 0,
                                color: !animating ? '#304050' : '#ff2d78',
                                border: '1px solid',
                                borderColor: !animating ? '#1a2a34' : '#ff2d7850',
                                borderRadius: 1.5,
                                p: 1,
                                bgcolor: !animating ? 'transparent' : '#ff2d7810',
                                boxShadow: !animating ? 'none' : '0 0 10px #ff2d7830',
                                '&:hover': { bgcolor: '#ff2d7820' },
                            }}
                        >
                            <StopIcon />
                        </IconButton>
                    </span>
                </Tooltip>
            </Toolbar>
        </AppBar>
    );
};

export default TopBar;
