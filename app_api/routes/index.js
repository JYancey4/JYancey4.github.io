/*
 * Route Definitions
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * Defines the routes for the Express application, mapping HTTPrequests to corresponding controller actions. It facilitates user authentication and trip management functionalities.

 * Key Features:
 * - Mapping of HTTP routes to controller actions
 * - Authentication handling with JWT
 * - Trip management operations
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Initial creation with route definitions for login, registration, and trip management.
 */

// Defines routes that map HTTP requests to controller functions

const express = require('express');
const router = express.Router();
const jwt = require('express-jwt');

const auth = jwt({
    secret: process.env.JWT_SECRET,
    userProperty: 'payload',
    algorithms: ["HS256"],
}); 

const authController = require('../controllers/authentication');
const tripsController = require('../controllers/trips');

router
    .route('/login')
    .post(authController.login);

router
    .route('/register')
    .post(authController.register);

router
    .route('/trips')
    .get(tripsController.tripsList)
    .post(auth, tripsController.tripsAddTrip);


router
    .route('/trips/:tripCode')
    .get(tripsController.tripsFindCode)
    .put(auth, tripsController.tripsUpdateTrip)
    .delete (auth, tripsController.tripsDeleteTrip);

module.exports = router;
