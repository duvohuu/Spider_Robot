import React, { createContext, useContext, useRef, useState, useEffect, useCallback } from 'react';

const WsContext = createContext(null);

const WsProvider = ({ children }) => {
    const ws = useRef(null);
    const [connected, setConnected] = useState(false);
    const [robotState, setRobotState] = useState('idle'); // 'idle' | 'running'
    const [feetData, setFeetData] = useState(null);
    const [latency, setLatency] = useState(null);
    const [distCm, setDistCm] = useState(null);  // khoảng cách SRF05 (cm), null nếu chưa có
    const [url, setUrl] = useState('ws://192.168.4.1/ws');
    const pingTs = useRef(null);

    // ==================== CONNECT ====================

    const connect = useCallback((targetUrl) => {
        if (ws.current) ws.current.close();
        const socket = new WebSocket(targetUrl || url);

        socket.onopen = () => {
            setConnected(true);
            pingTs.current = Date.now();
            socket.send(JSON.stringify({ cmd: 'ping' }));
        };

        socket.onclose = () => {
            setConnected(false);
            setRobotState('idle');
            setDistCm(null);
        };

        socket.onerror = () => {
            setConnected(false);
        };

        socket.onmessage = (e) => {
            try {
                const msg = JSON.parse(e.data);
                if (msg.type === 'feet') {
                    setRobotState(msg.state);
                    setFeetData(msg.feet);
                }
                if (msg.type === 'state') setRobotState(msg.state);
                if (msg.type === 'pong' && pingTs.current) {
                    setLatency(Date.now() - pingTs.current);
                    // Schedule next ping every 2s
                    setTimeout(() => {
                        if (ws.current?.readyState === WebSocket.OPEN) {
                            pingTs.current = Date.now();
                            ws.current.send(JSON.stringify({ cmd: 'ping' }));
                        }
                    }, 2000);
                }
                if (msg.dist_cm !== undefined) setDistCm(msg.dist_cm);
            } catch { /* ignore non-JSON */ }
        };

        ws.current = socket;
    }, [url]);

    const disconnect = useCallback(() => {
        ws.current?.close();
    }, []);

    const send = useCallback((obj) => {
        if (ws.current?.readyState === WebSocket.OPEN) {
            ws.current.send(JSON.stringify(obj));
        }
    }, []);

    // ==================== CLEANUP ====================

    useEffect(() => () => ws.current?.close(), []);

    // ==================== RENDER ====================

    return (
        <WsContext.Provider value={{
            connected, robotState, feetData, latency, distCm,
            url, setUrl,
            connect, disconnect, send,
        }}>
            {children}
        </WsContext.Provider>
    );
};

const useWs = () => useContext(WsContext);

export { WsProvider, useWs };
