/*
 * Trips Controller
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * This file contains the controller logic for handling CRUD operations related to 'trips' 
 * within the MongoDB database. It facilitates the listing, creation, updating, 
 * and deletion of trip information.
 *
 * Key Features:
 * - CRUD operations for 'trips'
 * - Interaction with MongoDB using Mongoose
 * - Route handling for trip-related requests
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Initial setup for CRUD operations on 'trips'.
 */

// Interact with MongoDB using Mongoose. Handles logic for 'tripsList' and 'tripsFindCode'

const mongoose = require('mongoose'); // .set('debug', true);
const Trip = mongoose.model('trips');
const User = mongoose.model('users');

// GET: /trips - lists all the trips
const tripsList = async (req, res) => {
    Trip
        .find({}) // empty filter for all
        .exec((err, trips) => {
            if (!trips) {
                return res
                    .status(404)
                    .json({ "message": "trips not found" });
            } else if (err) {
                return res
                    .status(404)
                    .json(err);
            } else {
                return res
                    .status(200)
                    .json(trips);
            }
        });
};

// GET /trips/:tripCode - returns a single trip
const tripsFindCode = async (req, res) => {
    Trip
        .find({ 'code': req.params.tripCode })
        .exec((err, trip) => {
            if (!trip) {
                return res
                    .status(404)
                    .json({ "message": "trip not found" });
            } else if (err) {
                return res
                    .status(404)
                    .json(err);
            } else {
                return res
                    .status(200)
                    .json(trip);
            }
        });
};

const tripsAddTrip = async (req, res) => {
    getUser(req, res,
        (req, res) => {
            Trip
                .create({
                    code: req.body.code,
                    name: req.body.name,
                    length: req.body.length,
                    start: req.body.start,
                    resort: req.body.resort,
                    perPerson: req.body.perPerson,
                    image: req.body.image,
                    description: req.body.description
                },
                    (err, trip) => {
                        if (err) {
                            return res
                                .status(400) // bad request
                                .json(err);
                        } else {
                            return res
                                .status(201) // created
                                .json(trip);
                        }
                    });
        }
    );
}

const tripsUpdateTrip = async (req, res) => {
    getUser(req, res,
        (req, res) => {
            Trip
                .findOneAndUpdate({ 'code': req.params.tripCode }, {
                    code: req.body.code,
                    name: req.body.name,
                    length: req.body.length,
                    start: req.body.start,
                    resort: req.body.resort,
                    perPerson: req.body.perPerson,
                    image: req.body.image,
                    description: req.body.description
                }, { new: true })
                .then(trip => {
                    if (!trip) {
                        return res
                            .status(404)
                            .send({
                                message: "Trip not found with code" + req.params.tripCode
 });
                    }
                    res.send(trip);
                }).catch(err => {
                    if (err.kind === 'ObjectId') {
                        return res
                            .status(404)
                            .send({
                                message: "Trip not found with code" + req.params.tripCode
 });
                    }
                    return res
                        .status(500) // server error
                        .json(err);
                });
        }
    );
}

const getUser = (req, res, callback) => {
    if (req.payload && req.payload.email) {
        User.findOne({ email: req.payload.email })
            .exec((err, user) => {
                if (!user) {
                    return res
                        .status(404)
                        .json({ "message": "User not found" });
                } else if (err) {
                    console.log(err);
                    return res
                        .status(404)
                        .json(err);
                }
                callback(req, res, user.name);
            });
    } else {
        return res
            .status(404)
            .json({ "message": "User not found" });
    }
};

const tripsDeleteTrip = async (req, res) => {
    if (req.payload && req.payload.email) {
        User.findOne({ email: req.payload.email })
            .exec((err, user) => {
                if (!user) {
                    return res.status(404).json({ "message": "User not authorized or not found" });
                } else if (err) {
                    console.log(err);
                    return res.status(404).json(err);
                } else {
                    Trip.findOneAndDelete({ 'code': req.params.tripCode })
                        .exec((err, trip) => {
                            if (err) {
                                return res.status(400).json(err); // bad request
                            } else if (!trip) {
                                return res.status(404).json({ "message": "Trip not found" });
                            } else {
                                return res.status(204).json(null); // no content, successful deletion
                            }
                        });
                }
            });
    } else {
        return res.status(401).json({ "message": "User not authorized" });
    }
};

module.exports = {
    tripsList,
    tripsFindCode,
    tripsAddTrip,
    tripsUpdateTrip,
    tripsDeleteTrip
};

