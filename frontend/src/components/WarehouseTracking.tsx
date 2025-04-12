import React, { useState, useEffect, useRef } from 'react';
import { Box as MuiBox, Paper as MuiPaper, Typography, Tooltip, IconButton, Alert, Switch, CircularProgress } from '@mui/material';
import { styled } from '@mui/material/styles';
import RobotIcon from '@mui/icons-material/SmartToy';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import { useNavigate } from 'react-router-dom';

// Styled components
const TrackingContainer = styled(MuiPaper)(({ theme }) => ({
  height: '100vh',
  width: '100vw',
  position: 'fixed',
  top: 0,
  left: 0,
  margin: 0,
  padding: 0,
  background: '#ffffff',
  display: 'flex',
  flexDirection: 'column',
  overflow: 'hidden'
}));

const Track = styled(MuiBox)({
  position: 'absolute',
  backgroundColor: '#000',
  border: '2px solid #000'
});

const Tag = styled(MuiBox)<{ color: string }>(({ color }) => ({
  position: 'absolute',
  width: '80px',
  height: '30px',
  backgroundColor: 'white',
  border: '2px solid #000',
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
  justifyContent: 'center',
  '&::before': {
    content: '""',
    position: 'absolute',
    top: '50%',
    left: '50%',
    transform: 'translate(-50%, -50%)',
    width: '60px',
    height: '20px',
    backgroundColor: color,
    borderRadius: '4px'
  }
}));

const Robot = styled(MuiBox)(({ theme }) => ({
  position: 'absolute',
  width: '45px',
  height: '45px',
  transition: 'all 0.5s ease',
  zIndex: 10,
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  '& img': {
    width: '100%',
    height: '100%',
    transform: 'rotate(90deg)',
  }
}));

interface Position {
  x: number;
  y: number;
  rotation: number;
}

interface Checkpoint {
  id: string;
  x: number;
  y: number;
  nextCheckpoints: string[];
  tag?: string;
}

// Define all possible checkpoints in the circuit
const checkpoints: Checkpoint[] = [
  { 
    id: 'home', 
    x: 95, 
    y: 95, 
    nextCheckpoints: ['bottom_left'],
    tag: 'HOME'
  },
  { 
    id: 'bottom_left', 
    x: 95, 
    y: 290, 
    nextCheckpoints: ['home', 'bottom_mid1'],
    tag: 'TAG4'
  },
  { 
    id: 'bottom_mid1', 
    x: 140, 
    y: 290, 
    nextCheckpoints: ['bottom_left', 'top_mid1']
  },
  { 
    id: 'top_mid1', 
    x: 140, 
    y: 95, 
    nextCheckpoints: ['bottom_mid1', 'top_mid2'],
    tag: 'TAG1'
  },
  { 
    id: 'top_mid2', 
    x: 185, 
    y: 95, 
    nextCheckpoints: ['top_mid1', 'bottom_mid2']
  },
  { 
    id: 'bottom_mid2', 
    x: 185, 
    y: 290, 
    nextCheckpoints: ['top_mid2', 'bottom_right']
  },
  { 
    id: 'bottom_right', 
    x: 215, 
    y: 290, 
    nextCheckpoints: ['bottom_mid2', 'top_right']
  },
  { 
    id: 'top_right', 
    x: 215, 
    y: 95, 
    nextCheckpoints: ['bottom_right', 'home'],
    tag: 'TAG3'
  }
];

const WarehouseTracking = () => {
  const navigate = useNavigate();
  const [robotPosition, setRobotPosition] = useState<Position>({ x: 95, y: 95, rotation: 0 });
  const [robotStatus, setRobotStatus] = useState<'moving' | 'idle' | 'charging'>('idle');
  const [currentCheckpoint, setCurrentCheckpoint] = useState<string>('home');
  const [reachedCheckpoints, setReachedCheckpoints] = useState<string[]>(['home']);
  const [isDebugMode, setIsDebugMode] = useState(false);
  const [isTestMode, setIsTestMode] = useState(true);
  const [connectionStatus, setConnectionStatus] = useState<string>('Disconnected');
  const wsRef = useRef<WebSocket | null>(null);

  // Test mode movement between checkpoints
  useEffect(() => {
    if (isTestMode) {
      let checkpointIndex = 0;
      const interval = setInterval(() => {
        const nextCheckpoint = checkpoints[checkpointIndex];
        setRobotPosition({
          x: nextCheckpoint.x,
          y: nextCheckpoint.y,
          rotation: calculateRotation(robotPosition, nextCheckpoint)
        });
        setCurrentCheckpoint(nextCheckpoint.id);
        checkpointIndex = (checkpointIndex + 1) % checkpoints.length;
      }, 2000);

      return () => clearInterval(interval);
    }
  }, [isTestMode, robotPosition]);

  // WebSocket connection for live mode
  useEffect(() => {
    if (!isTestMode) {
      const connectWebSocket = () => {
        try {
          const ws = new WebSocket('ws://their-websocket-url');
          wsRef.current = ws;
          setConnectionStatus('Connecting...');

          ws.onopen = () => {
            console.log('Connected to robot tracking system');
            setConnectionStatus('Connected');
          };

          ws.onmessage = (event) => {
            try {
              const data = JSON.parse(event.data);
              const newPosition = {
                x: data.x,
                y: data.y,
                rotation: data.rotation
              };
              setRobotPosition(newPosition);
              
              // Check if we've reached any checkpoint
              const reachedCP = checkpoints.find(cp => 
                Math.abs(cp.x - newPosition.x) < 5 && 
                Math.abs(cp.y - newPosition.y) < 5
              );
              
              if (reachedCP && reachedCP.id !== currentCheckpoint) {
                setCurrentCheckpoint(reachedCP.id);
                if (!reachedCheckpoints.includes(reachedCP.id)) {
                  setReachedCheckpoints(prev => [...prev, reachedCP.id]);
                }
                if (reachedCP.tag) {
                  console.log(`Robot reached ${reachedCP.tag}`);
                }
              }

              if (data.status) {
                setRobotStatus(data.status);
              }
            } catch (error) {
              console.error('Failed to parse WebSocket message:', error);
            }
          };

          ws.onclose = () => {
            setConnectionStatus('Disconnected');
            setTimeout(connectWebSocket, 3000);
          };

          ws.onerror = () => {
            setConnectionStatus('Error');
          };
        } catch (error) {
          console.error('WebSocket connection failed:', error);
          setConnectionStatus('Error');
          setTimeout(connectWebSocket, 3000);
        }
      };

      connectWebSocket();
      return () => wsRef.current?.close();
    }
  }, [isTestMode, currentCheckpoint, reachedCheckpoints]);

  // Function to calculate rotation based on current and next position
  const calculateRotation = (current: Position, target: { x: number, y: number }): number => {
    if (current.x === target.x) {
      return current.y < target.y ? 180 : 0;
    }
    return current.x < target.x ? 90 : 270;
  };

  return (
    <TrackingContainer>
      {/* Header */}
      <MuiBox sx={{ 
        p: 2, 
        borderBottom: '1px solid rgba(0, 0, 0, 0.1)',
        backgroundColor: '#fff',
        display: 'flex', 
        alignItems: 'center'
      }}>
        <IconButton onClick={() => navigate('/')} sx={{ mr: 2 }}>
          <ArrowBackIcon />
        </IconButton>
        <Typography variant="h6" sx={{ fontWeight: 'bold' }}>
          WAREHOUSE MAP
        </Typography>
        <MuiBox sx={{ ml: 'auto', display: 'flex', alignItems: 'center', gap: 2 }}>
          {/* Test Mode Toggle */}
          <MuiBox sx={{ display: 'flex', alignItems: 'center' }}>
            <Typography variant="caption" sx={{ mr: 1 }}>
              Test Mode
            </Typography>
            <Switch
              size="small"
              checked={isTestMode}
              onChange={(e) => setIsTestMode(e.target.checked)}
            />
          </MuiBox>

          {/* Connection Status */}
          {!isTestMode && (
            <MuiBox sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
              {connectionStatus === 'Connecting...' && (
                <CircularProgress size={16} sx={{ color: 'warning.main' }} />
              )}
              <Typography 
                variant="caption" 
                sx={{ 
                  color: connectionStatus === 'Connected' ? 'success.main' : 
                         connectionStatus === 'Connecting...' ? 'warning.main' : 'error.main'
                }}
              >
                {connectionStatus.toUpperCase()}
              </Typography>
            </MuiBox>
          )}

          {/* Debug Toggle */}
          <IconButton 
            onClick={() => setIsDebugMode(!isDebugMode)}
            sx={{ color: isDebugMode ? 'primary.main' : 'text.secondary' }}
          >
            <Typography variant="caption">Debug</Typography>
          </IconButton>
        </MuiBox>
      </MuiBox>

      {/* SVG Map Container */}
      <MuiBox sx={{ 
        flex: 1,
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
        p: 4,
        backgroundColor: '#f5f5f5'
      }}>
        <svg
          viewBox="0 0 600 700"
          style={{
            maxWidth: '90vw',
            maxHeight: '80vh',
            backgroundColor: 'white',
            borderRadius: '12px',
            boxShadow: '0 4px 20px rgba(0,0,0,0.1)'
          }}
        >
          {/* Outer maze walls */}
          <rect x="50" y="50" width="500" height="600" stroke="black" strokeWidth="5" fill="none" />

          {/* Vertical partition */}
          <line x1="300" y1="50" x2="300" y2="300" stroke="black" strokeWidth="5" />

          {/* Inner wall */}
          <rect x="200" y="300" width="200" height="200" stroke="black" strokeWidth="5" fill="none" />

          {/* Home square */}
          <rect x="50" y="50" width="100" height="100" stroke="black" strokeWidth="5" fill="white" />
          <text x="100" y="120" fontSize="16" textAnchor="middle" fill="black">home</text>
          
          {/* Tag boxes */}
          <rect x="220" y="10" width="160" height="40" stroke="black" strokeWidth="2" fill="white" />
          <circle cx="300" cy="30" r="15" fill="red" />
          <text x="300" y="65" fontSize="14" textAnchor="middle">TAG 1</text>

          <rect x="510" y="200" width="40" height="160" stroke="black" strokeWidth="2" fill="white" />
          <circle cx="530" cy="280" r="15" fill="yellow" />
          <text x="530" y="370" fontSize="14" textAnchor="middle">TAG 2</text>

          <rect x="400" y="530" width="100" height="40" stroke="black" strokeWidth="2" fill="white" />
          <circle cx="450" cy="550" r="15" fill="blue" />
          <text x="450" y="590" fontSize="14" textAnchor="middle">TAG 3</text>

          <rect x="100" y="530" width="100" height="40" stroke="black" strokeWidth="2" fill="white" />
          <circle cx="150" cy="550" r="15" fill="green" />
          <text x="150" y="590" fontSize="14" textAnchor="middle">TAG 4</text>

          {/* Measurements */}
          <text x="10" y="350" fontSize="16" transform="rotate(-90,10,350)">1.2 m</text>
          <text x="270" y="680" fontSize="16">45</text>
          <text x="420" y="680" fontSize="16">30</text>
          <text x="120" y="680" fontSize="16">45</text>
          <text x="270" y="270" fontSize="16">50 cm</text>

          {/* Robot */}
          <g 
            transform={`translate(${robotPosition.x} ${robotPosition.y}) rotate(${robotPosition.rotation})`}
            style={{ transition: 'transform 0.5s ease' }}
          >
            <image
              href="/robot-black.png"
              x="-20"
              y="-20"
              width="40"
              height="40"
            />
          </g>

          {/* Debug points */}
          {isDebugMode && checkpoints.map((cp, index) => (
            <circle
              key={index}
              cx={cp.x}
              cy={cp.y}
              r="5"
              fill={reachedCheckpoints.includes(cp.id) ? 'rgba(0,255,0,0.5)' : 'rgba(255,0,0,0.5)'}
            />
          ))}
        </svg>
      </MuiBox>
    </TrackingContainer>
  );
};

export default WarehouseTracking; 