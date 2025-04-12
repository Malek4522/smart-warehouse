const Task = require('../models/taskmodel');

// Get all tasks
exports.getAllTasks = async (req, res) => {
    try {
        const tasks = await Task.find();
        res.status(200).json({
            success: true,
            count: tasks.length,
            data: tasks
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get a single task by ID
exports.getTaskById = async (req, res) => {
    try {
        const task = await Task.findById(req.params.id);
        
        if (!task) {
            return res.status(404).json({
                success: false,
                error: 'Task not found'
            });
        }
        
        res.status(200).json({
            success: true,
            data: task
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Create a new task
exports.createTask = async (req, res) => {
    try {
        const task = await Task.create(req.body);
        
        res.status(201).json({
            success: true,
            data: task
        });
    } catch (error) {
        if (error.name === 'ValidationError') {
            const messages = Object.values(error.errors).map(err => err.message);
            
            return res.status(400).json({
                success: false,
                error: messages
            });
        } else {
            res.status(500).json({
                success: false,
                error: 'Server Error'
            });
        }
    }
};

// Update a task
exports.updateTask = async (req, res) => {
    try {
        const task = await Task.findByIdAndUpdate(
            req.params.id,
            req.body,
            {
                new: true,
                runValidators: true
            }
        );
        
        if (!task) {
            return res.status(404).json({
                success: false,
                error: 'Task not found'
            });
        }
        
        res.status(200).json({
            success: true,
            data: task
        });
    } catch (error) {
        if (error.name === 'ValidationError') {
            const messages = Object.values(error.errors).map(err => err.message);
            
            return res.status(400).json({
                success: false,
                error: messages
            });
        } else {
            res.status(500).json({
                success: false,
                error: 'Server Error'
            });
        }
    }
};

// Delete a task
exports.deleteTask = async (req, res) => {
    try {
        const task = await Task.findByIdAndDelete(req.params.id);
        
        if (!task) {
            return res.status(404).json({
                success: false,
                error: 'Task not found'
            });
        }
        
        res.status(200).json({
            success: true,
            data: {}
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get tasks by status
exports.getTasksByStatus = async (req, res) => {
    try {
        const tasks = await Task.find({ status: req.params.status });
        
        res.status(200).json({
            success: true,
            count: tasks.length,
            data: tasks
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Get tasks by assigned user
exports.getTasksByUser = async (req, res) => {
    try {
        const tasks = await Task.find({ assignedTo: req.params.userId });
        
        res.status(200).json({
            success: true,
            count: tasks.length,
            data: tasks
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};

// Update task status
exports.updateTaskStatus = async (req, res) => {
    try {
        const task = await Task.findByIdAndUpdate(
            req.params.id,
            { status: req.body.status },
            {
                new: true,
                runValidators: true
            }
        );
        
        if (!task) {
            return res.status(404).json({
                success: false,
                error: 'Task not found'
            });
        }
        
        res.status(200).json({
            success: true,
            data: task
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: 'Server Error'
        });
    }
};
