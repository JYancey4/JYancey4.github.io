/*
 * User Routes
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * Manages routes for user-related functionalities such as user registration and retrieval. 
 * This file defines the endpoints for user operations within the application.
 *
 * Key Features:
 * - User registration and retrieval endpoints
 * - Mapping of user-related HTTP requests to controller actions
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Initial setup of user routes.
 */

var express = require('express');
var router = express.Router();

/* GET users listing. */
router.get('/', function(req, res, next) {
  res.send('respond with a resource');
});

module.exports = router;
