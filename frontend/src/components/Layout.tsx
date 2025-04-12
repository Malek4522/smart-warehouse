import React from 'react';
import { Box, AppBar, Toolbar, Typography, IconButton, Container } from '@mui/material';
import { useNavigate } from 'react-router-dom';
import MenuIcon from '@mui/icons-material/Menu';
import HomeIcon from '@mui/icons-material/Home';
import GpsFixedIcon from '@mui/icons-material/GpsFixed';
import DashboardIcon from '@mui/icons-material/Dashboard';

// Import logo using require
const logo = require('../assets/images/logo.png');

interface LayoutProps {
  children: React.ReactNode;
}

const Layout: React.FC<LayoutProps> = ({ children }) => {
  const navigate = useNavigate();

  return (
    <Box sx={{ 
      display: 'flex', 
      flexDirection: 'column', 
      minHeight: '100vh',
      background: 'linear-gradient(135deg, #1a237e 0%, #0a1929 100%)',
      color: 'white'
    }}>
      {/* AppBar with Logo */}
      <AppBar position="fixed" sx={{ 
        zIndex: (theme) => theme.zIndex.drawer + 1,
        background: 'rgba(26, 35, 126, 0.8)',
        backdropFilter: 'blur(10px)'
      }}>
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
          
          {/* Logo */}
          <Box
            component="img"
            src={logo}
            alt="Warehouse Logo"
            sx={{
              height: 40,
              mr: 2
            }}
          />

          <Typography variant="h6" noWrap component="div" sx={{ flexGrow: 1 }}>
            Warehouse Tracking System
          </Typography>

          {/* Navigation Controls */}
          <Box sx={{ display: 'flex', gap: 2, alignItems: 'center' }}>
            <IconButton 
              onClick={() => navigate('/')} 
              color="inherit"
              sx={{
                bgcolor: 'primary.main',
                '&:hover': {
                  bgcolor: 'primary.dark',
                }
              }}
            >
              <HomeIcon />
            </IconButton>
            <IconButton 
              onClick={() => navigate('/dashboard')} 
              color="inherit"
              sx={{
                '&:hover': {
                  backgroundColor: 'rgba(255, 255, 255, 0.1)',
                }
              }}
            >
              <DashboardIcon />
            </IconButton>
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
          </Box>
        </Toolbar>
      </AppBar>

      {/* Main Content */}
      <Box component="main" sx={{ 
        flexGrow: 1,
        mt: '64px', // Height of AppBar
        p: 3
      }}>
        <Container maxWidth="xl">
          {children}
        </Container>
      </Box>
    </Box>
  );
};

export default Layout; 