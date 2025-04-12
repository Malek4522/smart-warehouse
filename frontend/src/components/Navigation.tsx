import React from 'react';
import { AppBar, Toolbar, Typography, Button, Box, Container, Link } from '@mui/material';
import { useNavigate, useLocation } from 'react-router-dom';
import DashboardIcon from '@mui/icons-material/Dashboard';
import GpsFixedIcon from '@mui/icons-material/GpsFixed';
import HomeIcon from '@mui/icons-material/Home';

const Navigation: React.FC = () => {
  const navigate = useNavigate();
  const location = useLocation();

  const isActive = (path: string) => location.pathname === path;

  return (
    <>
      {/* Navigation Bar */}
      <AppBar 
        position="fixed" 
        sx={{ 
          background: 'white',
          boxShadow: '0 2px 10px rgba(0,0,0,0.1)',
          zIndex: (theme) => theme.zIndex.drawer + 1
        }}
      >
        <Container maxWidth="lg">
          <Toolbar sx={{ justifyContent: 'space-between' }}>
            <Typography 
              variant="h6" 
              component="div" 
              sx={{ 
                color: '#1F2937',
                fontWeight: 'bold',
                display: 'flex',
                alignItems: 'center',
                gap: 1
              }}
            >
              <Box sx={{ 
                width: 8, 
                height: 8, 
                borderRadius: '50%', 
                bgcolor: '#00FF9D',
                mr: 1
              }} />
              Warehouse System
            </Typography>

            <Box sx={{ display: 'flex', gap: 2 }}>
              <Button
                startIcon={<HomeIcon />}
                onClick={() => navigate('/welcome')}
                sx={{
                  color: isActive('/welcome') ? '#00FF9D' : '#6B7280',
                  fontWeight: isActive('/welcome') ? 'bold' : 'normal',
                  '&:hover': {
                    backgroundColor: 'rgba(0, 255, 157, 0.1)',
                  }
                }}
              >
                Welcome
              </Button>
              <Button
                startIcon={<DashboardIcon />}
                onClick={() => navigate('/')}
                sx={{
                  color: isActive('/') ? '#00FF9D' : '#6B7280',
                  fontWeight: isActive('/') ? 'bold' : 'normal',
                  '&:hover': {
                    backgroundColor: 'rgba(0, 255, 157, 0.1)',
                  }
                }}
              >
                Dashboard
              </Button>
              <Button
                startIcon={<GpsFixedIcon />}
                onClick={() => navigate('/tracking')}
                sx={{
                  color: isActive('/tracking') ? '#00FF9D' : '#6B7280',
                  fontWeight: isActive('/tracking') ? 'bold' : 'normal',
                  '&:hover': {
                    backgroundColor: 'rgba(0, 255, 157, 0.1)',
                  }
                }}
              >
                Tracking
              </Button>
            </Box>
          </Toolbar>
        </Container>
      </AppBar>

      {/* Footer */}
      <Box
        component="footer"
        sx={{
          py: 3,
          px: 2,
          mt: 'auto',
          backgroundColor: 'white',
          borderTop: '1px solid #E5E7EB'
        }}
      >
        <Container maxWidth="lg">
          <Box sx={{ 
            display: 'flex', 
            justifyContent: 'space-between',
            alignItems: 'center',
            flexWrap: 'wrap',
            gap: 2
          }}>
            <Typography variant="body2" color="text.secondary">
              © {new Date().getFullYear()} Warehouse System. All rights reserved.
            </Typography>
            <Box sx={{ display: 'flex', gap: 3 }}>
              <Link href="#" color="inherit" underline="hover">
                Privacy Policy
              </Link>
              <Link href="#" color="inherit" underline="hover">
                Terms of Service
              </Link>
              <Link href="#" color="inherit" underline="hover">
                Contact Us
              </Link>
            </Box>
          </Box>
        </Container>
      </Box>
    </>
  );
};

export default Navigation; 