/*
 * Travel Controller
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * Handles travel-related functionalities, including retrieving and displaying a list of trips 
 * from the database. It interacts with the model to fetch travel data and renders it to the user.
 *
 * Key Features:
 * - Fetching travel data from the database
 * - Rendering travel list to the view
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Initial implementation of travel data retrieval and view rendering.
 */

const fs = require('fs');
// Load the trips data from the JSON file
const trips = JSON.parse(fs.readFileSync('./data/trips.json', 'utf8'));
// Required module for making HTTP requests
const request = require('request');

const apiOptions = {
    server: 'http://localhost:3000'
};

// render travel list view
const renderTravelList = (req, res, responseBody) => {
    let message = null;
    let pageTitle = process.env.npm_package_description + ' - Travel';

    if (!(responseBody instanceof Array)) {
        message = 'API lookup error';
        responseBody = [];
    } else {
        if (!responseBody.length) {
            message = 'No trips exist in database';
        }
    }

    res.render('travel', {
        title: pageTitle,
        trips: responseBody,
        message
    });
};


// GET travel list view
const travelList = (req, res) => {
    const path = '/api/trips';
    const requestOptions = {
        url: `${apiOptions.server}${path}`,
        method: 'GET',
        json: {},
    };

    console.info('>> travelController.travelList calling ' + requestOptions.url);

    request(
        requestOptions,
        (err, { statusCode }, body) => {
            if (err) {
                console.error(err);
            }
            renderTravelList(req, res, body);
        }
    );
};

module.exports = {
    travelList
};
