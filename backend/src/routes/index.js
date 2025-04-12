const express = require('express');
const router = express.Router();
const shelfController = require('../controllers/shelfcontroller');

// Health check endpoint
router.get('/health', (req, res) => {
  res.json({
    status: 'healthy',
    timestamp: new Date().toISOString(),
    uptime: process.uptime(),
    memoryUsage: process.memoryUsage(),
    environment: process.env.NODE_ENV || 'development'
  });
});

// Shelf routes - getting information
router.get('/shelves', shelfController.getAllShelves);
router.get('/shelves/color/:color', shelfController.getShelvesByColor);
router.get('/shelves/:shelfNumber', shelfController.getShelfByNumber);
router.get('/positions/color/:color', shelfController.getPositionsByColor);
router.get('/shelves/:shelfNumber/levels/:levelNumber', shelfController.getPositionsByLevel);
router.get('/shelves/:shelfNumber/levels/:levelNumber/positions/:positionNumber', shelfController.getPositionDetails);

// Finding available positions
router.get('/positions/available/:color', shelfController.findAvailablePosition);
router.get('/positions/filled', shelfController.getFilledPositions);

// Statistics
router.get('/warehouse/stats', shelfController.getWarehouseStats);

// Shelf routes - modifying positions
router.post('/shelves/:shelfNumber/levels/:levelNumber/positions/:positionNumber/place', shelfController.placeItem);
router.post('/shelves/:shelfNumber/levels/:levelNumber/positions/:positionNumber/remove', shelfController.removeItem);
router.post('/shelves/:shelfNumber/levels/:levelNumber/positions/:positionNumber/reserve', shelfController.reservePosition);

module.exports = router; 