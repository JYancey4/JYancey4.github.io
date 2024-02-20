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

const express = require('express');
const router = express.Router();
const ctrlMain = require('../controllers/main');

/* GET home page. */
router.get('/', ctrlMain.index);

module.exports = router;
