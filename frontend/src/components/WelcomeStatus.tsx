import React, { useState } from 'react';
import { Box as MuiBox, Paper, Typography, Grid, Select, MenuItem, TextField, FormControl, InputLabel, SelectChangeEvent } from '@mui/material';
import ColorLensIcon from '@mui/icons-material/ColorLens';
import Inventory2Icon from '@mui/icons-material/Inventory2';
import PeopleIcon from '@mui/icons-material/People';
import { styled } from '@mui/material/styles';

// Interface for queue status data
interface QueueStatusData {
  clientsAhead: number;
  estimatedTime: number;
}

// Interface for zone data
interface ZoneData {
  id: string;
  name: string;
  color: string;
}

// Props interface for the component
interface WelcomeStatusProps {
  queueStatus?: QueueStatusData;
  zones?: ZoneData[];
  onZoneSelect?: (zoneId: string) => void;
  onQuantityChange?: (quantity: string) => void;
}

// Zone colors configuration
const zoneColors = [
  { id: 'red', name: 'Red Zone', color: '#f44336' },
  { id: 'blue', name: 'Blue Zone', color: '#2196f3' },
  { id: 'yellow', name: 'Yellow Zone', color: '#ffd700' },
  { id: 'green', name: 'Green Zone', color: '#4caf50' }
];

const ServiceCard = styled(Paper)(({ theme }) => ({
  padding: theme.spacing(4),
  height: '100%',
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
  justifyContent: 'center',
  gap: theme.spacing(2),
  borderRadius: theme.spacing(2),
  background: 'rgba(255, 255, 255, 0.05)',
  backdropFilter: 'blur(10px)',
  border: '2px solid #00FF9D',
  cursor: 'pointer',
  transition: 'all 0.3s ease',
  '&:hover': {
    transform: 'translateY(-5px)',
    boxShadow: '0 8px 24px rgba(0, 255, 157, 0.1)',
  }
}));

const QueueCard = styled(Paper)(({ theme }) => ({
  padding: theme.spacing(4),
  marginTop: theme.spacing(4),
  borderRadius: theme.spacing(2),
  background: 'linear-gradient(135deg, #8B5CF6 0%, #6D28D9 100%)',
  color: 'white',
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  boxShadow: '0 4px 20px rgba(139, 92, 246, 0.3)'
}));

const IconWrapper = styled(MuiBox)(({ theme }) => ({
  width: '48px',
  height: '48px',
  borderRadius: '50%',
  background: 'rgba(0, 255, 157, 0.1)',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  marginBottom: theme.spacing(2),
  '& svg': {
    color: '#00FF9D',
    fontSize: '24px'
  }
}));

const WelcomeStatus: React.FC<WelcomeStatusProps> = ({
  queueStatus = { clientsAhead: 3, estimatedTime: 4 },
  zones = zoneColors,
  onZoneSelect,
  onQuantityChange
}) => {
  const [selectedColor, setSelectedColor] = useState<string>('');
  const [quantity, setQuantity] = useState<string>('');

  const handleColorChange = (event: SelectChangeEvent<unknown>) => {
    const value = event.target.value as string;
    setSelectedColor(value);
    onZoneSelect?.(value);
  };

  const handleQuantityChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value;
    if (value === '' || (parseInt(value) >= 1 && parseInt(value) <= 999)) {
      setQuantity(value);
      onQuantityChange?.(value);
    }
  };

  return (
    <MuiBox sx={{ 
      background: 'linear-gradient(135deg, #1a237e 0%, #0a1929 100%)',
      minHeight: '100vh',
      p: 3,
      color: 'white'
    }}>
      <MuiBox sx={{ 
        maxWidth: '1200px', 
        mx: 'auto',
        py: 4
      }}>
        <MuiBox sx={{ 
          mb: 5, 
          position: 'relative',
          textAlign: 'center',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center'
        }}>
          <Typography 
            variant="h4" 
            sx={{ 
              fontWeight: 'bold',
              color: 'white',
              mb: 2,
              textShadow: '0 2px 4px rgba(0,0,0,0.2)'
            }}
          >
            👋 Welcome! Please select your product details
          </Typography>
          <Typography
            variant="h6"
            sx={{
              color: 'rgba(255, 255, 255, 0.7)',
              mb: 3,
              fontWeight: 600,
              maxWidth: '600px',
              mx: 'auto'
            }}
          >
            Fill in the details below to proceed with your order
          </Typography>
          <MuiBox
            sx={{
              width: '180px',
              height: '6px',
              background: '#00FF9D',
              borderRadius: '3px',
              boxShadow: '0 0 20px rgba(0, 255, 157, 0.3)'
            }}
          />
        </MuiBox>

        <Grid container spacing={3}>
          {/* Product Selection Card */}
          <Grid item xs={12} sm={6}>
            <ServiceCard>
              <IconWrapper>
                <ColorLensIcon />
              </IconWrapper>
              <Typography variant="h6" sx={{ color: 'white', fontWeight: 600 }}>
                Select Product
              </Typography>
              <Typography variant="body2" sx={{ color: 'rgba(255, 255, 255, 0.7)', textAlign: 'center' }}>
                Choose a product
              </Typography>
              <FormControl fullWidth sx={{ mt: 2 }}>
                <Select
                  value={selectedColor}
                  onChange={handleColorChange}
                  displayEmpty
                  sx={{
                    borderRadius: '8px',
                    background: 'rgba(255, 255, 255, 0.1)',
                    '& .MuiOutlinedInput-notchedOutline': {
                      borderColor: 'rgba(255, 255, 255, 0.2)'
                    },
                    '& .MuiSelect-select': {
                      padding: '12px',
                      color: 'white',
                      '&::placeholder': {
                        color: 'rgba(255, 255, 255, 0.5)'
                      }
                    },
                    '& .MuiSvgIcon-root': {
                      color: 'white'
                    }
                  }}
                >
                  <MenuItem disabled value="">
                    <em style={{ color: 'rgba(255, 255, 255, 0.5)' }}>
                      Select a product zone (e.g., Red, Blue, Yellow, Green)
                    </em>
                  </MenuItem>
                  {zones.map(zone => (
                    <MenuItem key={zone.id} value={zone.id}>
                      <MuiBox sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                        <MuiBox
                          sx={{
                            width: 16,
                            height: 16,
                            borderRadius: '50%',
                            backgroundColor: zone.color
                          }}
                        />
                        <Typography sx={{ color: 'white' }}>
                          {zone.name}
                        </Typography>
                      </MuiBox>
                    </MenuItem>
                  ))}
                </Select>
              </FormControl>
            </ServiceCard>
          </Grid>

          {/* Quantity Card */}
          <Grid item xs={12} sm={6}>
            <ServiceCard>
              <IconWrapper>
                <Inventory2Icon />
              </IconWrapper>
              <Typography variant="h6" sx={{ color: 'white', fontWeight: 600 }}>
                Set Quantity
              </Typography>
              <Typography variant="body2" sx={{ color: 'rgba(255, 255, 255, 0.7)', textAlign: 'center' }}>
                Enter your quantity
              </Typography>
              <FormControl fullWidth sx={{ mt: 2 }}>
                <TextField
                  type="number"
                  value={quantity}
                  onChange={handleQuantityChange}
                  placeholder="Specify how many items you need"
                  inputProps={{ 
                    min: 1,
                    max: 999,
                    style: { 
                      textAlign: 'center',
                      color: 'white'
                    }
                  }}
                  sx={{
                    '& .MuiOutlinedInput-root': {
                      borderRadius: '8px',
                      background: 'rgba(255, 255, 255, 0.1)',
                      '& fieldset': {
                        borderColor: 'rgba(255, 255, 255, 0.2)'
                      },
                      '& input': {
                        padding: '12px',
                        color: 'white',
                        '&::placeholder': {
                          color: 'rgba(255, 255, 255, 0.5)'
                        }
                      }
                    }
                  }}
                />
              </FormControl>
            </ServiceCard>
          </Grid>
        </Grid>

        {/* Queue Status */}
        <QueueCard>
          <MuiBox sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
            <PeopleIcon sx={{ fontSize: 24 }} />
            <MuiBox>
              <Typography variant="body2" sx={{ opacity: 0.8 }}>
                Clients left before you
              </Typography>
              <Typography variant="h4" fontWeight="bold">
                {queueStatus.clientsAhead}
              </Typography>
            </MuiBox>
          </MuiBox>

          <MuiBox sx={{ textAlign: 'right' }}>
            <Typography variant="body2" sx={{ opacity: 0.8 }}>
              You will be served in
            </Typography>
            <Typography variant="h4" fontWeight="bold">
              {queueStatus.estimatedTime} minutes
            </Typography>
          </MuiBox>
        </QueueCard>
      </MuiBox>
    </MuiBox>
  );
};

export default WelcomeStatus;