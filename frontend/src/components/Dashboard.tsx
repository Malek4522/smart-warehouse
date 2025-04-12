import React, { useState, useEffect } from 'react';
import { Box, Grid, Paper, Typography, IconButton, useTheme, Tooltip, AppBar, Toolbar, Container } from '@mui/material';
import { styled } from '@mui/material/styles';
import { ResponsivePie } from '@nivo/pie';
import { ResponsiveBar } from '@nivo/bar';
import { useNavigate } from 'react-router-dom';
import Brightness4Icon from '@mui/icons-material/Brightness4';
import Brightness7Icon from '@mui/icons-material/Brightness7';
import TrendingUpIcon from '@mui/icons-material/TrendingUp';
import InventoryIcon from '@mui/icons-material/Inventory';
import LocalShippingIcon from '@mui/icons-material/LocalShipping';
import ShoppingCartIcon from '@mui/icons-material/ShoppingCart';
import PaidIcon from '@mui/icons-material/Paid';
import NewReleasesIcon from '@mui/icons-material/NewReleases';
import ThermostatIcon from '@mui/icons-material/Thermostat';
import GpsFixedIcon from '@mui/icons-material/GpsFixed';
import MenuIcon from '@mui/icons-material/Menu';
import HomeIcon from '@mui/icons-material/Home';

// Styled components
const StyledPaper = styled(Paper)(({ theme }) => ({
  padding: theme.spacing(4),
  color: 'white',
  background: 'rgba(255, 255, 255, 0.05)',
  backdropFilter: 'blur(10px)',
  height: '100%',
  borderRadius: theme.spacing(2),
  border: '1px solid rgba(255, 255, 255, 0.1)',
  transition: 'all 0.3s ease',
  '&:hover': {
    transform: 'translateY(-5px)',
    boxShadow: '0 8px 24px rgba(0, 255, 157, 0.1)',
  }
}));

const StatCard = styled(Paper)<{ customcolor: string }>(({ theme, customcolor }) => ({
  padding: theme.spacing(4),
  textAlign: 'center',
  color: 'white',
  background: 'rgba(255, 255, 255, 0.05)',
  backdropFilter: 'blur(10px)',
  borderRadius: theme.spacing(2),
  border: '1px solid rgba(255, 255, 255, 0.1)',
  transition: 'all 0.3s ease',
  position: 'relative',
  overflow: 'hidden',
  '&:hover': {
    transform: 'translateY(-5px)',
    boxShadow: `0 8px 24px ${customcolor}44`,
    '&::before': {
      opacity: 0.2,
    },
    '&::after': {
      opacity: 0.1,
    }
  },
  '&::before': {
    content: '""',
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    background: customcolor,
    opacity: 0,
    transition: 'opacity 0.3s ease',
    zIndex: 0,
  },
  '&::after': {
    content: '""',
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    background: 'linear-gradient(45deg, rgba(0,0,0,0.2) 0%, rgba(0,0,0,0.4) 100%)',
    opacity: 0,
    transition: 'opacity 0.3s ease',
    zIndex: 0,
  },
  '& > *': {
    position: 'relative',
    zIndex: 1,
  }
}));

// Temperature monitoring component
const TemperatureDisplay = styled(Box)(({ theme }) => ({
  display: 'flex',
  alignItems: 'center',
  gap: theme.spacing(2),
  padding: theme.spacing(2, 3),
  borderRadius: theme.spacing(2),
  background: 'rgba(255, 255, 255, 0.05)',
  backdropFilter: 'blur(10px)',
  border: '1px solid rgba(255, 255, 255, 0.1)',
}));

// Zone data for section overview
const zoneData = [
  { 
    id: 'Red Zone', 
    baseColor: '#f44336',  // Red zone
    lightColor: '#ef9a9a', // Light red for free
    darkColor: '#b71c1c'   // Dark red for occupied
  },
  { 
    id: 'Blue Zone', 
    baseColor: '#2196f3',  // Blue zone
    lightColor: '#90caf9', // Light blue for free
    darkColor: '#0d47a1'   // Dark blue for occupied
  },
  { 
    id: 'Yellow Zone', 
    baseColor: '#ffd700',  // Yellow zone
    lightColor: '#fff59d', // Light yellow for free
    darkColor: '#fbc02d'   // Dark yellow for occupied
  },
  { 
    id: 'Green Zone', 
    baseColor: '#4caf50',  // Green zone
    lightColor: '#81c784', // Light green for free
    darkColor: '#1b5e20'   // Dark green for occupied
  },
];

// Mock occupation data (true = occupied, false = free)
const sectionOccupation = [
  // Red Zone (12 sections)
  true, false, true, true, false, true, false, true, true, false, true, false,
  // Blue Zone (12 sections)
  false, true, true, false, true, false, true, false, true, true, false, true,
  // Yellow Zone (12 sections)
  true, true, false, true, false, true, false, true, false, true, true, false,
  // Green Zone (12 sections)
  false, true, false, true, true, false, true, false, true, false, true, true
];

// Stats data with colors
const statsData = [
  { 
    icon: <TrendingUpIcon sx={{ color: '#00FF9D' }} />, 
    value: '(23%)33', 
    label: 'Return Customers', 
    color: '#3f51b5' 
  },
  { 
    icon: <InventoryIcon sx={{ color: '#00FF9D' }} />, 
    value: '1942', 
    label: 'Low Stock', 
    color: '#f44336' 
  },
  { 
    icon: <LocalShippingIcon sx={{ color: '#00FF9D' }} />, 
    value: '2453', 
    label: 'Units Sold', 
    color: '#4caf50' 
  },
  { 
    icon: <ShoppingCartIcon sx={{ color: '#00FF9D' }} />, 
    value: '4125', 
    label: 'Total Orders', 
    color: '#ff9800' 
  },
  { 
    icon: <PaidIcon sx={{ color: '#00FF9D' }} />, 
    value: '532k $', 
    label: 'Sales', 
    color: '#2196f3' 
  },
  { 
    icon: <NewReleasesIcon sx={{ color: '#00FF9D' }} />, 
    value: '3432', 
    label: 'New Orders', 
    color: '#9c27b0' 
  },
];

// Mock data for the pie chart
const usageData = [
  { id: 'used', label: 'Used Space', value: 58, color: '#2196f3' },
  { id: 'free', label: 'Free Space', value: 42, color: '#90caf9' },
];

// Mock data for the bar chart
const inventoryData = [
  { month: 'Jul', value: 400 },
  { month: 'Aug', value: 300 },
  { month: 'Sep', value: 600 },
  { month: 'Oct', value: 800 },
  { month: 'Nov', value: 500 },
];

const bestSellers = [
  { name: 'Product A', units: 124, revenue: '$17,933' },
  { name: 'Product B', units: 107, revenue: '$15,114' },
  { name: 'Product C', units: 102, revenue: '$12,940' },
];

// Updated section data structure to represent shelves with places
const generateShelfData = () => {
  const shelvesPerZone = 3; // 3 rows of shelves
  const placesPerShelf = 5; // 5 places in each shelf
  
  return zoneData.map(zone => ({
    ...zone,
    shelves: Array.from({ length: shelvesPerZone }).map((_, shelfIndex) => ({
      id: `${zone.id}-${shelfIndex + 1}`,
      places: Array.from({ length: placesPerShelf }).map((_, placeIndex) => ({
        id: `${zone.id}-${shelfIndex + 1}-${placeIndex + 1}`,
        isOccupied: Math.random() > 0.5, // Random occupation for demo
        number: placeIndex + 1
      }))
    }))
  }));
};

// Mock temperature data for different zones
const initialTemperatures = {
  A: 22.5,
  B: 23.1,
  C: 21.8,
  D: 22.3
};

// Function to get temperature status color
const getTemperatureColor = (temp: number) => {
  if (temp < 18) return '#2196f3'; // Too cold - blue
  if (temp > 25) return '#f44336'; // Too hot - red
  return '#4caf50'; // Normal - green
};

// Temperature type definition
type Temperatures = {
  A: number;
  B: number;
  C: number;
  D: number;
};

interface SensorData {
  zoneId: keyof Temperatures;
  temperature: number;
}

interface SensorWebSocket {
  connect: (onMessage: (data: SensorData) => void) => void;
  disconnect: () => void;
}

class MockSensorWebSocket implements SensorWebSocket {
  private intervalId: NodeJS.Timeout | null = null;
  private onMessageCallback: ((data: SensorData) => void) | null = null;

  connect(onMessage: (data: SensorData) => void): void {
    this.onMessageCallback = onMessage;
    this.intervalId = setInterval(() => {
      const zones: Array<keyof Temperatures> = ['A', 'B', 'C', 'D'];
      const randomZone = zones[Math.floor(Math.random() * zones.length)];
      const randomTemp = 18 + Math.random() * 10;
      
      if (this.onMessageCallback) {
        this.onMessageCallback({
          zoneId: randomZone,
          temperature: parseFloat(randomTemp.toFixed(1))
        });
      }
    }, 2000);
  }

  disconnect(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = null;
    }
  }
}

// Mock function to fetch initial sensor data
const fetchSensorData = async (): Promise<SensorData[]> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve([
        { zoneId: 'A', temperature: 22.5 },
        { zoneId: 'B', temperature: 23.1 },
        { zoneId: 'C', temperature: 21.8 },
        { zoneId: 'D', temperature: 22.3 },
      ]);
    }, 1000);
  });
};

const Dashboard = () => {
  const theme = useTheme();
  const navigate = useNavigate();
  const [mode, setMode] = React.useState<'light' | 'dark'>('light');
  const [temperatures, setTemperatures] = useState<Temperatures>({
    A: 22.5,
    B: 23.1,
    C: 21.8,
    D: 22.3
  });
  const [sensorConnection, setSensorConnection] = useState<SensorWebSocket | null>(null);

  // Initialize WebSocket connection and fetch initial data
  useEffect(() => {
    // Fetch initial data
    const fetchInitialData = async () => {
      try {
        const initialData = await fetchSensorData();
        updateTemperaturesFromSensorData(initialData);
      } catch (error) {
        console.error('Failed to fetch initial sensor data:', error);
      }
    };

    // Set up WebSocket connection for real-time updates
    const ws = new MockSensorWebSocket();
    ws.connect((data) => {
      updateTemperaturesFromSensorData([data]);
    });

    setSensorConnection(ws);
    fetchInitialData();

    // Cleanup WebSocket connection on component unmount
    return () => {
      if (ws) {
        ws.disconnect();
      }
    };
  }, []);

  // Function to update temperatures from sensor data
  const updateTemperaturesFromSensorData = (data: SensorData[]) => {
    setTemperatures((prev) => {
      const newTemps = { ...prev };
      data.forEach((reading) => {
        newTemps[reading.zoneId] = reading.temperature;
      });
      return newTemps;
    });
  };

  const toggleColorMode = () => {
    setMode((prevMode) => (prevMode === 'light' ? 'dark' : 'light'));
  };

  return (
    <Box sx={{ 
      display: 'flex', 
      flexDirection: 'column', 
      minHeight: '100vh',
      background: 'linear-gradient(135deg, #1a237e 0%, #0a1929 100%)',
      color: 'white'
    }}>
      {/* AppBar */}
      <AppBar position="fixed" sx={{ zIndex: (theme) => theme.zIndex.drawer + 1 }}>
        <Toolbar>
          <IconButton
            color="inherit"
            aria-label="open drawer"
            edge="start"
            onClick={() => {}}
            sx={{ mr: 2 }}
          >
            <MenuIcon />
          </IconButton>
          <Typography variant="h6" noWrap component="div" sx={{ flexGrow: 1 }}>
            Warehouse Dashboard
          </Typography>

          {/* Navigation Controls */}
          <Box sx={{ display: 'flex', gap: 2, alignItems: 'center' }}>
            <Tooltip title="Product Status">
              <IconButton 
                onClick={() => navigate('/welcome')} 
                color="inherit"
                sx={{
                  bgcolor: 'primary.main',
                  '&:hover': {
                    bgcolor: 'primary.dark',
                  },
                  '& .MuiSvgIcon-root': {
                    color: 'background.default'
                  }
                }}
              >
                <HomeIcon />
              </IconButton>
            </Tooltip>
            <Tooltip title="Robot Tracking">
              <IconButton 
                onClick={() => navigate('/tracking')} 
                color="inherit"
                sx={{
                  '&:hover': {
                    backgroundColor: 'rgba(255, 255, 255, 0.1)',
                  }
                }}
              >
                <GpsFixedIcon />
              </IconButton>
            </Tooltip>
            <IconButton 
              onClick={toggleColorMode} 
              color="inherit"
              sx={{
                '&:hover': {
                  backgroundColor: 'rgba(255, 255, 255, 0.1)',
                }
              }}
            >
              {mode === 'dark' ? <Brightness7Icon /> : <Brightness4Icon />}
            </IconButton>
          </Box>
        </Toolbar>
      </AppBar>

      {/* Main content */}
      <Box sx={{ 
        display: 'flex', 
        flexGrow: 1,
        mt: '64px' // Height of AppBar
      }}>
        {/* Sidebar */}
        {/* ... keep existing sidebar code ... */}

        {/* Main content area */}
        <Box component="main" sx={{ 
          flexGrow: 1,
          display: 'flex',
          flexDirection: 'column',
          overflow: 'auto'
        }}>
          {/* Rest of dashboard content */}
          <Box sx={{ p: 4, pt: 8 }}>
            <Container maxWidth="xl">
              {/* Navigation Bar with Temperature Display */}
              <Box sx={{ 
                display: 'flex', 
                justifyContent: 'space-between', 
                alignItems: 'center', 
                mb: 4,
                flexWrap: 'wrap',
                gap: 3
              }}>
                {/* Temperature Monitors */}
                <Box sx={{ 
                  display: 'flex', 
                  gap: 2,
                  flexWrap: 'wrap'
                }}>
                  {Object.entries(temperatures).map(([zone, temp]) => (
                    <Tooltip
                      key={zone}
                      title={`Zone ${zone} Temperature Status: ${
                        temp < 18 ? 'Too Cold' : temp > 25 ? 'Too Hot' : 'Normal'
                      }`}
                    >
                      <TemperatureDisplay>
                        <ThermostatIcon sx={{ color: getTemperatureColor(temp) }} />
                        <Typography variant="body2">
                          Zone {zone}:
                        </Typography>
                        <Typography
                          variant="body2"
                          sx={{ 
                            color: getTemperatureColor(temp),
                            fontWeight: 'bold'
                          }}
                        >
                          {temp}°C
                        </Typography>
                      </TemperatureDisplay>
                    </Tooltip>
                  ))}
                </Box>

                {/* Navigation Controls */}
                <Box sx={{ display: 'flex', gap: 1 }}>
                  <Tooltip title="Go to Robot Tracking">
                    <IconButton onClick={() => navigate('/tracking')} color="primary">
                      <GpsFixedIcon />
                    </IconButton>
                  </Tooltip>
                  <IconButton onClick={toggleColorMode} color="inherit">
                    {mode === 'dark' ? <Brightness7Icon /> : <Brightness4Icon />}
                  </IconButton>
                </Box>
              </Box>

              {/* Stats Row */}
              <Grid container spacing={4} sx={{ mb: 4 }}>
                {statsData.map((stat, index) => (
                  <Grid item xs={12} sm={6} md={2} key={index}>
                    <StatCard customcolor={stat.color}>
                      <Box sx={{ mb: 2, color: stat.color }}>{stat.icon}</Box>
                      <Typography variant="h5" sx={{ color: stat.color }}>
                        {stat.value}
                      </Typography>
                      <Typography variant="subtitle1">{stat.label}</Typography>
                    </StatCard>
                  </Grid>
                ))}
              </Grid>

              {/* Main Content */}
              <Grid container spacing={4}>
                {/* Location Usage */}
                <Grid item xs={12} md={4}>
                  <StyledPaper>
                    <Typography variant="h5" gutterBottom>
                      Section Usage
                    </Typography>
                    <Box sx={{ height: 400, background: 'rgba(255, 255, 255, 0.05)', borderRadius: 2, p: 3 }}>
                      <ResponsivePie
                        data={usageData}
                        margin={{ top: 40, right: 80, bottom: 80, left: 80 }}
                        innerRadius={0.5}
                        padAngle={0.7}
                        cornerRadius={3}
                        activeOuterRadiusOffset={8}
                        borderWidth={1}
                        borderColor={{ from: 'color', modifiers: [['darker', 0.2]] }}
                        arcLinkLabelsSkipAngle={10}
                        arcLinkLabelsTextColor="#ffffff"
                        arcLinkLabelsThickness={2}
                        arcLinkLabelsColor={{ from: 'color' }}
                        arcLabelsSkipAngle={10}
                        arcLabelsTextColor={{ from: 'color', modifiers: [['darker', 2]] }}
                        defs={[
                          {
                            id: 'dots',
                            type: 'patternDots',
                            background: 'inherit',
                            color: 'rgba(255, 255, 255, 0.3)',
                            size: 4,
                            padding: 1,
                            stagger: true
                          },
                          {
                            id: 'lines',
                            type: 'patternLines',
                            background: 'inherit',
                            color: 'rgba(255, 255, 255, 0.3)',
                            rotation: -45,
                            lineWidth: 6,
                            spacing: 10
                          }
                        ]}
                        fill={[
                          {
                            match: {
                              id: 'used'
                            },
                            id: 'dots'
                          },
                          {
                            match: {
                              id: 'free'
                            },
                            id: 'lines'
                          }
                        ]}
                      />
                    </Box>
                  </StyledPaper>
                </Grid>

                {/* Inventory Chart */}
                <Grid item xs={12} md={8}>
                  <StyledPaper>
                    <Typography variant="h5" gutterBottom>
                      Inventory Stock
                    </Typography>
                    <Box sx={{ height: 400 }}>
                      <ResponsiveBar
                        data={inventoryData}
                        keys={['value']}
                        indexBy="month"
                        margin={{ top: 50, right: 60, bottom: 50, left: 60 }}
                        padding={0.3}
                        valueScale={{ type: 'linear' }}
                        colors={{ scheme: 'set3' }}
                        borderColor={{ from: 'color', modifiers: [['darker', 1.6]] }}
                        axisTop={null}
                        axisRight={null}
                        labelSkipWidth={12}
                        labelSkipHeight={12}
                        role="application"
                      />
                    </Box>
                  </StyledPaper>
                </Grid>

                {/* Best Sellers */}
                <Grid item xs={12} md={4}>
                  <StyledPaper>
                    <Typography variant="h5" gutterBottom>
                      Best Sellers This Quarter
                    </Typography>
                    <Box sx={{ mt: 3 }}>
                      {bestSellers.map((seller, index) => (
                        <Box
                          key={index}
                          sx={{
                            p: 3,
                            mb: 2,
                            bgcolor: 'rgba(255, 255, 255, 0.03)',
                            borderRadius: 1,
                            border: '1px solid rgba(255, 255, 255, 0.1)',
                          }}
                        >
                          <Typography variant="h6">{seller.name}</Typography>
                          <Typography variant="body1" color="text.secondary">
                            Units: {seller.units} | Revenue: {seller.revenue}
                          </Typography>
                        </Box>
                      ))}
                    </Box>
                  </StyledPaper>
                </Grid>

                {/* Section Overview */}
                <Grid item xs={12} md={8}>
                  <StyledPaper>
                    <Typography variant="h5" gutterBottom align="center">
                      Section Overview
                    </Typography>
                    <Box sx={{ 
                      display: 'flex', 
                      justifyContent: 'center',
                      width: '100%',
                      maxWidth: '1600px',
                      margin: '0 auto'
                    }}>
                      <Grid container spacing={4} sx={{ maxWidth: '95%' }}>
                        {generateShelfData().map((zone) => (
                          <Grid item xs={12} sm={6} key={zone.id}>
                            <Box sx={{ 
                              mb: 3,
                              background: 'rgba(255, 255, 255, 0.02)',
                              borderRadius: '16px',
                              padding: '24px',
                              boxShadow: '0 4px 30px rgba(0, 0, 0, 0.1)',
                              backdropFilter: 'blur(5px)',
                              border: '1px solid rgba(255, 255, 255, 0.05)'
                            }}>
                              <Typography variant="h6" sx={{ 
                                mb: 3, 
                                color: zone.baseColor, 
                                textAlign: 'center',
                                fontSize: '1.2rem',
                                fontWeight: 'bold'
                              }}>
                                {zone.id}
                              </Typography>
                              <Grid container spacing={2}>
                                {zone.shelves.map((shelf, shelfIndex) => (
                                  <Grid item xs={12} key={shelf.id}>
                                    <Box sx={{ mb: 1 }}>
                                      <Typography variant="caption" sx={{ color: zone.baseColor }}>
                                        Shelf {shelfIndex + 1}
                                      </Typography>
                                    </Box>
                                    <Grid container spacing={1} sx={{ justifyContent: 'center' }}>
                                      {shelf.places.map((place) => (
                                        <Grid item xs={2} key={place.id}>
                                          <Tooltip 
                                            title={`Shelf ${shelfIndex + 1}, Place ${place.number} - ${place.isOccupied ? 'Occupied' : 'Free'}`}
                                            placement="top"
                                          >
                                            <Box
                                              sx={{
                                                position: 'relative',
                                                width: '100%',
                                                paddingTop: '100%',
                                                bgcolor: place.isOccupied ? zone.darkColor : zone.lightColor,
                                                borderRadius: '8px',
                                                transition: 'all 0.3s ease',
                                                opacity: 0.9,
                                                border: '2px solid rgba(255, 255, 255, 0.1)',
                                                '&:hover': {
                                                  opacity: 1,
                                                  transform: 'scale(1.1)',
                                                  boxShadow: '0 4px 8px rgba(0,0,0,0.3)',
                                                  zIndex: 1
                                                },
                                              }}
                                            >
                                              <Typography
                                                variant="caption"
                                                sx={{
                                                  position: 'absolute',
                                                  top: '50%',
                                                  left: '50%',
                                                  transform: 'translate(-50%, -50%)',
                                                  color: 'white',
                                                  fontSize: '0.85rem',
                                                  fontWeight: 'bold',
                                                  textShadow: '1px 1px 2px rgba(0,0,0,0.5)',
                                                  lineHeight: 1,
                                                }}
                                              >
                                                {place.number}
                                              </Typography>
                                            </Box>
                                          </Tooltip>
                                        </Grid>
                                      ))}
                                    </Grid>
                                  </Grid>
                                ))}
                              </Grid>
                            </Box>
                          </Grid>
                        ))}
                      </Grid>
                    </Box>
                    {/* Zone Legend */}
                    <Box sx={{ mt: 4, display: 'flex', gap: 3, justifyContent: 'center', flexWrap: 'wrap' }}>
                      {zoneData.map((zone) => (
                        <Box key={zone.id} sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                          <Typography variant="subtitle2" sx={{ 
                            minWidth: '100px',
                            color: zone.baseColor
                          }}>
                            {zone.id}:
                          </Typography>
                          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Box
                              sx={{
                                width: 12,
                                height: 12,
                                bgcolor: zone.darkColor,
                                borderRadius: '4px',
                              }}
                            />
                            <Typography variant="caption">Occupied</Typography>
                          </Box>
                          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Box
                              sx={{
                                width: 12,
                                height: 12,
                                bgcolor: zone.lightColor,
                                borderRadius: '4px',
                              }}
                            />
                            <Typography variant="caption">Free</Typography>
                          </Box>
                        </Box>
                      ))}
                    </Box>
                  </StyledPaper>
                </Grid>
              </Grid>
            </Container>
          </Box>
        </Box>
      </Box>
    </Box>
  );
};

export default Dashboard; 