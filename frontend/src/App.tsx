import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';
import Layout from './components/Layout';
import Tracking from './components/Tracking';
import Dashboard from './components/Dashboard';
import WelcomeStatus from './components/WelcomeStatus';

const theme = createTheme({
  palette: {
    mode: 'dark',
    primary: {
      main: '#00FF9D',
    },
    secondary: {
      main: '#8B5CF6',
    },
    background: {
      default: '#0a1929',
      paper: '#1a237e',
    },
  },
  typography: {
    fontFamily: '"Roboto", "Helvetica", "Arial", sans-serif',
  },
});

function App() {
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <Router>
        <Layout>
          <Routes>
            <Route path="/" element={<WelcomeStatus />} />
            <Route path="/dashboard" element={<Dashboard />} />
            <Route path="/tracking" element={<Tracking />} />
          </Routes>
        </Layout>
      </Router>
    </ThemeProvider>
  );
}

export default App; 