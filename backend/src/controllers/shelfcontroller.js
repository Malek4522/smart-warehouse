const Shelf = require('../models/shelfmodel');

// Initialize the 4 fixed shelves if they don't exist
exports.initializeShelves = async () => {
    try {
        const defaultShelves = [
            { 
                shelfNumber: 'S1',
                shelfColor: 'red',
                levels: [
                    { 
                        levelNumber: 1, 
                        positions: [
                            { positionNumber: 1, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'red', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 2, 
                        positions: [
                            { positionNumber: 1, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'red', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 3, 
                        positions: [
                            { positionNumber: 1, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'red', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'red', isEmpty: true, status: 'empty' }
                        ]
                    }
                ]
            },
            { 
                shelfNumber: 'S2',
                shelfColor: 'blue',
                levels: [
                    { 
                        levelNumber: 1, 
                        positions: [
                            { positionNumber: 1, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'blue', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 2, 
                        positions: [
                            { positionNumber: 1, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'blue', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 3, 
                        positions: [
                            { positionNumber: 1, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'blue', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'blue', isEmpty: true, status: 'empty' }
                        ]
                    }
                ]
            },
            { 
                shelfNumber: 'S3',
                shelfColor: 'green',
                levels: [
                    { 
                        levelNumber: 1, 
                        positions: [
                            { positionNumber: 1, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'green', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 2, 
                        positions: [
                            { positionNumber: 1, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'green', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 3, 
                        positions: [
                            { positionNumber: 1, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'green', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'green', isEmpty: true, status: 'empty' }
                        ]
                    }
                ]
            },
            { 
                shelfNumber: 'S4',
                shelfColor: 'yellow',
                levels: [
                    { 
                        levelNumber: 1, 
                        positions: [
                            { positionNumber: 1, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'yellow', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 2, 
                        positions: [
                            { positionNumber: 1, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'yellow', isEmpty: true, status: 'empty' }
                        ]
                    },
                    { 
                        levelNumber: 3, 
                        positions: [
                            { positionNumber: 1, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 2, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 3, color: 'yellow', isEmpty: true, status: 'empty' },
                            { positionNumber: 4, color: 'yellow', isEmpty: true, status: 'empty' }
                        ]
                    }
                ]
            }
        ];

        for (const shelfData of defaultShelves) {
            const existingShelf = await Shelf.findOne({ shelfNumber: shelfData.shelfNumber });
            if (!existingShelf) {
                await Shelf.create(shelfData);
            } else {
                await Shelf.findOneAndUpdate(
                    { shelfNumber: shelfData.shelfNumber },
                    shelfData,
                    { new: true }
                );
            }
        }
        console.log('Shelves initialized successfully');
    } catch (error) {
        console.error('Error initializing shelves:', error);
    }
};

// Get all shelves
exports.getAllShelves = async (req, res) => {
    try {
        const shelves = await Shelf.find();
        res.status(200).json({
            success: true,
            count: shelves.length,
            data: shelves
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get a single shelf by shelf number
exports.getShelfByNumber = async (req, res) => {
    try {
        const shelf = await Shelf.findOne({ shelfNumber: req.params.shelfNumber });
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        res.status(200).json({
            success: true,
            data: shelf
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get shelves by color
exports.getShelvesByColor = async (req, res) => {
    try {
        const { color } = req.params;
        const shelves = await Shelf.find({ shelfColor: color.toLowerCase() });
        
        res.status(200).json({
            success: true,
            count: shelves.length,
            data: shelves
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get positions by color
exports.getPositionsByColor = async (req, res) => {
    try {
        const { color } = req.params;
        const shelves = await Shelf.find();
        
        const positions = [];
        
        shelves.forEach(shelf => {
            shelf.levels.forEach(level => {
                level.positions.forEach(position => {
                    if (position.color === color.toLowerCase()) {
                        positions.push({
                            shelfNumber: shelf.shelfNumber,
                            levelNumber: level.levelNumber,
                            positionNumber: position.positionNumber,
                            color: position.color,
                            isEmpty: position.isEmpty,
                            status: position.status,
                            productId: position.productId
                        });
                    }
                });
            });
        });
        
        res.status(200).json({
            success: true,
            count: positions.length,
            data: positions
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get positions by level
exports.getPositionsByLevel = async (req, res) => {
    try {
        const { shelfNumber, levelNumber } = req.params;
        const shelf = await Shelf.findOne({ shelfNumber });
        
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        
        const level = shelf.levels.find(l => l.levelNumber === parseInt(levelNumber));
        
        if (!level) {
            return res.status(404).json({
                success: false,
                error: 'Level not found'
            });
        }
        
        res.status(200).json({
            success: true,
            count: level.positions.length,
            data: level.positions
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get position details
exports.getPositionDetails = async (req, res) => {
    try {
        const { shelfNumber, levelNumber, positionNumber } = req.params;
        const shelf = await Shelf.findOne({ shelfNumber });
        
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        
        const level = shelf.levels.find(l => l.levelNumber === parseInt(levelNumber));
        
        if (!level) {
            return res.status(404).json({
                success: false,
                error: 'Level not found'
            });
        }
        
        const position = level.positions.find(p => p.positionNumber === parseInt(positionNumber));
        
        if (!position) {
            return res.status(404).json({
                success: false,
                error: 'Position not found'
            });
        }
        
        res.status(200).json({
            success: true,
            data: position
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Place item in a position
exports.placeItem = async (req, res) => {
    try {
        const { shelfNumber, levelNumber, positionNumber } = req.params;
        const { productId, productData, color } = req.body;
        
        const shelf = await Shelf.findOne({ shelfNumber });
        
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        
        const level = shelf.levels.find(l => l.levelNumber === parseInt(levelNumber));
        
        if (!level) {
            return res.status(404).json({
                success: false,
                error: 'Level not found'
            });
        }
        
        const position = level.positions.find(p => p.positionNumber === parseInt(positionNumber));
        
        if (!position) {
            return res.status(404).json({
                success: false,
                error: 'Position not found'
            });
        }
        
        // Check that color matches if provided
        if (color && position.color !== color) {
            return res.status(400).json({
                success: false,
                error: `Position color (${position.color}) does not match product color (${color})`
            });
        }
        
        if (!position.isEmpty) {
            return res.status(400).json({
                success: false,
                error: 'Position is already occupied'
            });
        }
        
        // Update position with product data
        position.isEmpty = false;
        position.status = 'filled';
        position.productId = productId || `P${Date.now()}`;
        position.productData = productData || { color: position.color };
        position.lastUpdated = new Date();
        
        await shelf.save();
        
        res.status(200).json({
            success: true,
            data: position
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Remove item from a position
exports.removeItem = async (req, res) => {
    try {
        const { shelfNumber, levelNumber, positionNumber } = req.params;
        const shelf = await Shelf.findOne({ shelfNumber });
        
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        
        const level = shelf.levels.find(l => l.levelNumber === parseInt(levelNumber));
        
        if (!level) {
            return res.status(404).json({
                success: false,
                error: 'Level not found'
            });
        }
        
        const position = level.positions.find(p => p.positionNumber === parseInt(positionNumber));
        
        if (!position) {
            return res.status(404).json({
                success: false,
                error: 'Position not found'
            });
        }
        
        if (position.isEmpty) {
            return res.status(400).json({
                success: false,
                error: 'Position is already empty'
            });
        }
        
        // Get removed product info for response
        const removedProductId = position.productId;
        const removedProductData = position.productData;
        
        // Clear position
        position.isEmpty = true;
        position.status = 'empty';
        position.productId = null;
        position.productData = null;
        position.lastUpdated = new Date();
        
        await shelf.save();
        
        res.status(200).json({
            success: true,
            data: {
                shelfNumber,
                levelNumber,
                positionNumber,
                removedProductId,
                removedProductData
            }
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Reserve a position for a product
exports.reservePosition = async (req, res) => {
    try {
        const { shelfNumber, levelNumber, positionNumber } = req.params;
        const shelf = await Shelf.findOne({ shelfNumber });
        
        if (!shelf) {
            return res.status(404).json({
                success: false,
                error: 'Shelf not found'
            });
        }
        
        const level = shelf.levels.find(l => l.levelNumber === parseInt(levelNumber));
        
        if (!level) {
            return res.status(404).json({
                success: false,
                error: 'Level not found'
            });
        }
        
        const position = level.positions.find(p => p.positionNumber === parseInt(positionNumber));
        
        if (!position) {
            return res.status(404).json({
                success: false,
                error: 'Position not found'
            });
        }
        
        if (!position.isEmpty) {
            return res.status(400).json({
                success: false,
                error: 'Position is already occupied'
            });
        }
        
        // Reserve the position
        position.isEmpty = false;
        position.status = 'reserved';
        position.lastUpdated = new Date();
        
        await shelf.save();
        
        res.status(200).json({
            success: true,
            data: position
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Find available position for a product by color
exports.findAvailablePosition = async (req, res) => {
    try {
        const { color } = req.params;
        
        // Find shelves with matching color positions
        const shelves = await Shelf.find();
        
        // Find first available position
        let availablePosition = null;
        
        for (const shelf of shelves) {
            for (const level of shelf.levels) {
                for (const position of level.positions) {
                    if (position.color === color && position.isEmpty) {
                        availablePosition = {
                            shelfNumber: shelf.shelfNumber,
                            levelNumber: level.levelNumber,
                            positionNumber: position.positionNumber,
                            color: position.color
                        };
                        break;
                    }
                }
                if (availablePosition) break;
            }
            if (availablePosition) break;
        }
        
        if (!availablePosition) {
            return res.status(404).json({
                success: false,
                error: `No available positions found for color: ${color}`
            });
        }
        
        res.status(200).json({
            success: true,
            data: availablePosition
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get all filled positions
exports.getFilledPositions = async (req, res) => {
    try {
        const shelves = await Shelf.find();
        
        const filledPositions = [];
        
        shelves.forEach(shelf => {
            shelf.levels.forEach(level => {
                level.positions.forEach(position => {
                    if (!position.isEmpty) {
                        filledPositions.push({
                            shelfNumber: shelf.shelfNumber,
                            levelNumber: level.levelNumber,
                            positionNumber: position.positionNumber,
                            color: position.color,
                            status: position.status,
                            productId: position.productId,
                            productData: position.productData,
                            lastUpdated: position.lastUpdated
                        });
                    }
                });
            });
        });
        
        res.status(200).json({
            success: true,
            count: filledPositions.length,
            data: filledPositions
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get warehouse occupancy stats
exports.getWarehouseStats = async (req, res) => {
    try {
        const shelves = await Shelf.find();
        
        let totalPositions = 0;
        let filledPositions = 0;
        let reservedPositions = 0;
        let emptyPositions = 0;
        
        const colorStats = {
            red: { total: 0, filled: 0, empty: 0, reserved: 0 },
            blue: { total: 0, filled: 0, empty: 0, reserved: 0 },
            green: { total: 0, filled: 0, empty: 0, reserved: 0 },
            yellow: { total: 0, filled: 0, empty: 0, reserved: 0 }
        };
        
        shelves.forEach(shelf => {
            shelf.levels.forEach(level => {
                level.positions.forEach(position => {
                    totalPositions++;
                    colorStats[position.color].total++;
                    
                    if (!position.isEmpty) {
                        if (position.status === 'filled') {
                            filledPositions++;
                            colorStats[position.color].filled++;
                        } else if (position.status === 'reserved') {
                            reservedPositions++;
                            colorStats[position.color].reserved++;
                        }
                    } else {
                        emptyPositions++;
                        colorStats[position.color].empty++;
                    }
                });
            });
        });
        
        res.status(200).json({
            success: true,
            data: {
                totalPositions,
                filledPositions,
                reservedPositions,
                emptyPositions,
                occupancyRate: (filledPositions / totalPositions) * 100,
                colorStats
            }
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};
