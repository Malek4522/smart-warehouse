import React, { useState, useEffect, useRef } from 'react';
import { Box, Typography, Paper } from '@mui/material';
import { styled } from '@mui/material/styles';

// Import images using require
const carImage = require('../assets/images/car.png');
const trackImage = require('../assets/images/track.png');

const MapContainer = styled(Paper)(({ theme }) => ({
  position: 'relative',
  width: '100%',
  height: '700px',
  background: 'rgba(255, 255, 255, 0.05)',
  backdropFilter: 'blur(10px)',
  borderRadius: theme.spacing(2),
  overflow: 'hidden',
  border: '1px solid rgba(255, 255, 255, 0.1)',
  cursor: 'crosshair',
}));

const CarIcon = styled(Box)(({ theme }) => ({
  position: 'absolute',
  width: '30px',
  height: '50px',
  backgroundImage: `url(${carImage})`,
  backgroundSize: 'contain',
  backgroundRepeat: 'no-repeat',
  backgroundPosition: 'center',
  transition: 'all 0.5s ease',
  zIndex: 2,
}));

const WarehouseMap = styled(Box)(({ theme }) => ({
  position: 'absolute',
  width: '100%',
  height: '100%',
  backgroundImage: `url(${trackImage})`,
  backgroundSize: 'contain',
  backgroundRepeat: 'no-repeat',
  backgroundPosition: 'center',
  opacity: 1,
}));

interface Position {
  x: number;
  y: number;
  rotation: number;
}

interface ClickPosition {
  x: number;
  y: number;
}

// Define precise waypoints for the car to follow based on the track image
const pathPoints: Position[] = [
  // Starting position with initial movement options
  { x: 36.94, y: 22.22, rotation: 0 },
  { x: 36.94, y: 25.22, rotation: 0 }, // Forward
  { x: 36.94, y: 28.22, rotation: 0 }, // Forward
  { x: 36.94, y: 31.22, rotation: 0 }, // Forward
  { x: 36.88, y: 34.08, rotation: 180 }, // Slight left
  { x: 36.88, y: 37.08, rotation: 180 }, // Forward
  
  // First segment down with smooth transition
  { x: 36.88, y: 40.08, rotation: 180 }, // Forward
  { x: 36.88, y: 43.08, rotation: 180 }, // Forward
  { x: 36.88, y: 46.08, rotation: 180 }, // Forward
  { x: 36.88, y: 49.08, rotation: 180 }, // Forward
  { x: 36.88, y: 52.08, rotation: 180 }, // Forward
  { x: 36.88, y: 55.08, rotation: 180 }, // Forward
  { x: 36.88, y: 58.08, rotation: 180 }, // Forward
  { x: 36.88, y: 61.08, rotation: 180 }, // Forward
  { x: 36.88, y: 64.08, rotation: 180 }, // Forward
  { x: 36.94, y: 68.51, rotation: 180 }, // Forward
  
  // Right turn preparation
  { x: 38.4, y: 70.65, rotation: 135 }, // Start right turn
  { x: 40.4, y: 72.65, rotation: 90 }, // Continue right turn
  { x: 43.4, y: 74.65, rotation: 90 }, // Complete right turn
  { x: 43.4, y: 76.65, rotation: 90 }, // Forward
  
  // Right segment up
  { x: 45.5, y: 76.29, rotation: 90 }, // Forward
  { x: 47.5, y: 75.79, rotation: 90 }, // Forward
  
  // Left turn preparation
  { x: 47.5, y: 73.79, rotation: 45 }, // Start left turn
  { x: 47.5, y: 70.79, rotation: 0 }, // Continue left turn
  { x: 47.5, y: 67.79, rotation: 0 }, // Complete left turn
  { x: 47.71, y: 64.22, rotation: 0 }, // Forward
  { x: 47.71, y: 58.22, rotation: 0 }, // Forward
  { x: 47.71, y: 52.22, rotation: 0 }, // Forward
  
  // Upward movement
  { x: 50.36, y: 50.36, rotation: 0 }, // Forward
  { x: 52.36, y: 48.36, rotation: 0 }, // Forward
  
  // Right turn preparation
  { x: 55.36, y: 48.36, rotation: 45 }, // Start right turn
  { x: 57.36, y: 50.36, rotation: 90 }, // Continue right turn
  { x: 57.78, y: 55.79, rotation: 135 }, // Complete right turn
  { x: 57.78, y: 60.79, rotation: 180 }, // Forward
  
  // Downward movement
  { x: 57.78, y: 65.79, rotation: 180 }, // Forward
  { x: 57.78, y: 70.79, rotation: 180 }, // Forward
  { x: 57.36, y: 73.08, rotation: 180 }, // Forward
  { x: 57.36, y: 76.08, rotation: 180 }, // Forward
  
  // Right turn preparation
  { x: 60.19, y: 74.94, rotation: 225 }, // Start right turn
  { x: 62.19, y: 73.94, rotation: 270 }, // Continue right turn
  { x: 63.19, y: 71.94, rotation: 270 }, // Forward
  
  // Leftward movement
  { x: 63.19, y: 68.94, rotation: 270 }, // Forward
  { x: 63.19, y: 65.94, rotation: 270 }, // Forward
  { x: 63.19, y: 62.94, rotation: 270 }, // Forward
  { x: 63.19, y: 59.94, rotation: 270 }, // Forward
  { x: 63.19, y: 56.94, rotation: 270 }, // Forward
  { x: 63.19, y: 53.94, rotation: 270 }, // Forward
  { x: 63.19, y: 50.94, rotation: 270 }, // Forward
  { x: 63.54, y: 47.36, rotation: 270 }, // Forward
  
  // Left turn preparation
  { x: 63.54, y: 44.36, rotation: 315 }, // Start left turn
  { x: 63.54, y: 41.36, rotation: 0 }, // Continue left turn
  { x: 63.54, y: 38.36, rotation: 0 }, // Complete left turn
  { x: 63.54, y: 35.36, rotation: 0 }, // Forward
  { x: 63.54, y: 32.36, rotation: 0 }, // Forward
  { x: 63.54, y: 29.36, rotation: 0 }, // Forward
  { x: 62.78, y: 26.08, rotation: 0 }, // Forward
  
  // Upward movement
  { x: 60.78, y: 25.08, rotation: 0 }, // Forward
  { x: 58.78, y: 24.08, rotation: 0 }, // Forward
  { x: 56.78, y: 23.08, rotation: 0 }, // Forward
  { x: 54.78, y: 23.08, rotation: 0 }, // Forward
  { x: 52.71, y: 22.94, rotation: 0 }, // Forward
  
  // Final segment to starting position
  { x: 50.71, y: 22.94, rotation: 0 }, // Forward
  { x: 48.71, y: 22.94, rotation: 0 }, // Forward
  { x: 46.71, y: 22.94, rotation: 0 }, // Forward
  { x: 44.79, y: 21.65, rotation: 0 }, // Forward
  { x: 42.79, y: 21.65, rotation: 0 }, // Forward
  { x: 40.79, y: 21.65, rotation: 0 }, // Forward
  { x: 38.79, y: 21.65, rotation: 0 }, // Forward
  { x: 36.94, y: 22.22, rotation: 0 } // Back to start
];

const Tracking: React.FC = () => {
  const [carPosition, setCarPosition] = useState<Position>(pathPoints[0]);
  const [currentPointIndex, setCurrentPointIndex] = useState(0);
  const [isPaused, setIsPaused] = useState(false);
  const [clickPosition, setClickPosition] = useState<ClickPosition | null>(null);
  const mapRef = useRef<HTMLDivElement>(null);

  const handleMapClick = (event: React.MouseEvent<HTMLDivElement>) => {
    if (!mapRef.current) return;

    const rect = mapRef.current.getBoundingClientRect();
    const x = ((event.clientX - rect.left) / rect.width) * 100;
    const y = ((event.clientY - rect.top) / rect.height) * 100;

    setClickPosition({
      x: parseFloat(x.toFixed(2)),
      y: parseFloat(y.toFixed(2))
    });

    // Log the point in a format easy to copy
    console.log(`{ x: ${x.toFixed(2)}, y: ${y.toFixed(2)}, rotation: 0 },`);
  };

  useEffect(() => {
    if (isPaused) return;

    const moveToNextPoint = () => {
      setCurrentPointIndex((prevIndex) => {
        const nextIndex = (prevIndex + 1) % pathPoints.length;
        const targetPoint = pathPoints[nextIndex];
        setCarPosition(targetPoint);
        return nextIndex;
      });
    };

    const interval = setInterval(moveToNextPoint, 500);
    return () => clearInterval(interval);
  }, [isPaused]);

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h4" gutterBottom sx={{ color: 'white', mb: 4 }}>
        Live Warehouse Tracking
      </Typography>
      
      <MapContainer
        ref={mapRef}
        onClick={handleMapClick}
      >
        <WarehouseMap />
        <CarIcon
          sx={{
            left: `${carPosition.x}%`,
            top: `${carPosition.y}%`,
            transform: `translate(-50%, -50%) rotate(${carPosition.rotation}deg)`,
          }}
        />
        {clickPosition && (
          <Box
            sx={{
              position: 'absolute',
              left: `${clickPosition.x}%`,
              top: `${clickPosition.y}%`,
              width: '10px',
              height: '10px',
              backgroundColor: 'red',
              borderRadius: '50%',
              transform: 'translate(-50%, -50%)',
              zIndex: 3,
            }}
          />
        )}
      </MapContainer>

      <Box sx={{ mt: 3, display: 'flex', gap: 3 }}>
        <Paper sx={{ 
          p: 2, 
          flex: 1,
          background: 'rgba(255, 255, 255, 0.05)',
          backdropFilter: 'blur(10px)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
        }}>
          <Typography variant="h6" sx={{ color: 'white' }}>Click Coordinates</Typography>
          <Typography variant="body1" sx={{ color: 'white' }}>
            {clickPosition ? `X: ${clickPosition.x}, Y: ${clickPosition.y}` : 'Click on map to see coordinates'}
          </Typography>
        </Paper>
        
        <Paper sx={{ 
          p: 2, 
          flex: 1,
          background: 'rgba(255, 255, 255, 0.05)',
          backdropFilter: 'blur(10px)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
        }}>
          <Typography variant="h6" sx={{ color: 'white' }}>Current Location</Typography>
          <Typography variant="body1" sx={{ color: 'white' }}>
            {carPosition.y < 30 ? 'Home Area' :
             carPosition.x < 40 ? 'West Section' :
             carPosition.x > 60 ? 'East Section' :
             'Central Area'}
          </Typography>
        </Paper>
        
        <Paper 
          sx={{ 
            p: 2, 
            flex: 1,
            background: 'rgba(255, 255, 255, 0.05)',
            backdropFilter: 'blur(10px)',
            border: '1px solid rgba(255, 255, 255, 0.1)',
            cursor: 'pointer',
          }}
          onClick={() => setIsPaused(!isPaused)}
        >
          <Typography variant="h6" sx={{ color: 'white' }}>Vehicle Status</Typography>
          <Typography variant="body1" sx={{ color: 'white' }}>
            {isPaused ? 'Paused - Click to Resume' : 'Active - Click to Pause'}
          </Typography>
        </Paper>
      </Box>
    </Box>
  );
};

export default Tracking; 