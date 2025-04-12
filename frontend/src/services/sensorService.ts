// Type for sensor data
export interface SensorData {
  zoneId: string;
  temperature: number;
  timestamp: number;
}

// Example API configuration
const API_CONFIG = {
  baseUrl: process.env.REACT_APP_SENSOR_API_URL || 'your_sensor_api_url',
  apiKey: process.env.REACT_APP_SENSOR_API_KEY,
};

// Function to fetch real sensor data
export const fetchSensorData = async (): Promise<SensorData[]> => {
  try {
    const response = await fetch(`${API_CONFIG.baseUrl}/sensors/temperature`, {
      headers: {
        'Authorization': `Bearer ${API_CONFIG.apiKey}`,
        'Content-Type': 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error('Failed to fetch sensor data');
    }

    const data = await response.json();
    return data;
  } catch (error) {
    console.error('Error fetching sensor data:', error);
    throw error;
  }
};

// WebSocket connection for real-time updates
export class SensorWebSocket {
  private ws: WebSocket | null = null;
  private onDataCallback: ((data: SensorData[]) => void) | null = null;

  connect(onData: (data: SensorData[]) => void) {
    this.onDataCallback = onData;
    this.ws = new WebSocket(`${API_CONFIG.baseUrl.replace('http', 'ws')}/sensors/ws`);

    this.ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (this.onDataCallback) {
        this.onDataCallback(data);
      }
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };

    this.ws.onclose = () => {
      // Attempt to reconnect after 5 seconds
      setTimeout(() => this.connect(onData), 5000);
    };
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }
} 