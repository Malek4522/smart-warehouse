const mongoose = require('mongoose');

const positionSchema = new mongoose.Schema({
    positionNumber: {
        type: Number,
        required: true
    },
    color: {
        type: String,
        enum: ['red', 'blue', 'green', 'yellow'],
        required: true
    },
    isEmpty: {
        type: Boolean,
        default: true
    },
    productId: {
        type: String,
        default: null
    },
    productData: {
        type: Object,
        default: null
    },
    status: {
        type: String,
        enum: ['empty', 'filled', 'reserved', 'in_transit'],
        default: 'empty'
    },
    lastUpdated: {
        type: Date,
        default: Date.now
    }
});

const levelSchema = new mongoose.Schema({
    levelNumber: {
        type: Number,
        required: true
    },
    positions: [positionSchema]
});

const shelfSchema = new mongoose.Schema({
    shelfNumber: {
        type: String,
        required: true,
        unique: true
    },
    shelfColor: {
        type: String,
        enum: ['red', 'blue', 'green', 'yellow'],
        required: true
    },
    levels: [levelSchema]
});

const Shelf = mongoose.model('Shelf', shelfSchema);

module.exports = Shelf;

