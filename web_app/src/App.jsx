import React from 'react';
import { Box, Paper } from '@mui/material';
import TopBar from './components/layout/TopBar';
import SettingsPanel from './components/settings/SettingsPanel';
import SpiderViewer from './components/viewer/SpiderViewer';
import AngleDisplay from './components/layout/AngleDisplay';

const PANEL_BG     = '#04101d';
const PANEL_BORDER = '1px solid #0d2a3e';

const App = () => {
    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', height: '100vh', bgcolor: '#020b16', color: 'text.primary' }}>

            {/* Top bar */}
            <TopBar />

            {/* Main content */}
            <Box sx={{ display: 'flex', flex: 1, overflow: 'hidden' }}>

                {/* LEFT: Settings panel */}
                <Paper
                    elevation={0}
                    square
                    sx={{
                        width: 310,
                        minWidth: 270,
                        bgcolor: PANEL_BG,
                        borderRight: PANEL_BORDER,
                        overflow: 'auto',
                        flexShrink: 0,
                        '&::-webkit-scrollbar': { width: 4 },
                        '&::-webkit-scrollbar-thumb': { bgcolor: '#0d2a3e', borderRadius: 2 },
                    }}
                >
                    <SettingsPanel />
                </Paper>

                {/* CENTER: 3D viewer */}
                <Box sx={{ flex: 1, position: 'relative', overflow: 'hidden' }}>
                    <SpiderViewer />
                </Box>

                {/* RIGHT: Live angle readout */}
                <Paper
                    elevation={0}
                    square
                    sx={{
                        width: 240,
                        minWidth: 220,
                        bgcolor: PANEL_BG,
                        borderLeft: PANEL_BORDER,
                        overflow: 'auto',
                        flexShrink: 0,
                        '&::-webkit-scrollbar': { width: 4 },
                        '&::-webkit-scrollbar-thumb': { bgcolor: '#0d2a3e', borderRadius: 2 },
                    }}
                >
                    <AngleDisplay />
                </Paper>

            </Box>
        </Box>
    );
};

export default App;

