/*
 * Travel Routes
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * Defines routes for travel-related requests. It maps URLs to the appropriate controller 
 * functions for handling travel listings within the application.
 *
 * Key Features:
 * - Definition of routes for travel listings
 * - Integration with the travel controller for request handling
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Setup of travel-related routes.
 */

const express = require('express');
const router = express.Router();
const controller = require('../controllers/travel');

router.get('/', controller.travelList);

module.exports = router;