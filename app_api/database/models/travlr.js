/*
 * Trip Schema Definition
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * Defines the Mongoose schema for 'trips', outlining the structure of trip data 
 * within the MongoDB database. This schema is used for all database operations 
 * related to 'trips'.
 *
 * Key Features:
 * - Mongoose schema definition for 'trips'
 * - Required fields and data types for trip information
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Definition of the 'trips' schema.
 */

//File that defines the schema for trips data. Outlines the structure seen in MongoDB Compass

const mongoose = require('mongoose');
// define the trip schema
const tripSchema = new mongoose.Schema({
    code: { type: String, required: true, index: true },
    name: { type: String, required: true, index: true },
    length: { type: String, required: true },
    start: { type: Date, required: true },
    resort: { type: String, required: true },
    perPerson: { type: String, required: true },
    image: { type: String, required: true },
    description: { type: String, required: true }

});

//mongoose.model("trips", tripSchema):
module.exports = mongoose.model("trips", tripSchema);
