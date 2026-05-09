import React, { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { ThemeProvider, createTheme, CssBaseline } from '@mui/material';
import { WsProvider } from './context/WsContext';
import { RobotProvider } from './context/RobotContext';
import App from './App.jsx';

const theme = createTheme({
    palette: {
        mode: 'dark',
        primary:   { main: '#00e5ff' },
        secondary: { main: '#ff2d78' },
        warning:   { main: '#ffab40' },
        success:   { main: '#00e676' },
        error:     { main: '#ff2d78' },
        background: {
            default: '#020b16',
            paper:   '#05111f',
        },
        text: {
            primary:   '#d0eaf8',
            secondary: '#6a96b0',
        },
        divider: '#0d2a3e',
    },
    typography: {
        fontFamily: '"Inter", "Roboto", sans-serif',
        fontSize: 13,
    },
    shape: { borderRadius: 6 },
    components: {
        MuiSlider: {
            styleOverrides: {
                root:  { padding: '8px 0' },
                thumb: { width: 12, height: 12 },
                rail:  { opacity: 0.25 },
            },
        },
        MuiPaper: {
            styleOverrides: {
                root: { backgroundImage: 'none' },
            },
        },
        MuiAccordion: {
            styleOverrides: {
                root: { backgroundImage: 'none', backgroundColor: '#071726' },
            },
        },
        MuiChip: {
            styleOverrides: {
                root: { fontFamily: '"Inter", monospace', fontWeight: 700 },
            },
        },
    },
});

createRoot(document.getElementById('root')).render(
    <StrictMode>
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <WsProvider>
                <RobotProvider>
                    <App />
                </RobotProvider>
            </WsProvider>
        </ThemeProvider>
    </StrictMode>,
);
