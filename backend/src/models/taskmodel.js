const mongoose = require('mongoose');

const taskSchema = new mongoose.Schema({
    title: {
        type: String,
        required: true
    },
    robotId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'Robot',
        required: true
    },
    itemColor: {
        type: String,
        enum: ['red', 'yellow', 'green', 'blue'],
        required: true
    },
    timestamp: {
        type: Date,
        default: Date.now
    },
    status: {
        type: String,
        enum: ['active', 'finished', 'cancelled', 'error'],
        default: 'active',
        required: true
    }
});

const Task = mongoose.model('Task', taskSchema);

module.exports = Task;
