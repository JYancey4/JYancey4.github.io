/*
 * Passport Configuration
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * This file configures the Passport.js local authentication strategy, 
 * including the setup for user login authentication using email and password. 
 * It utilizes MongoDB for storing and validating user credentials.
 *
 * Key Features:
 * - Configuration of the local authentication strategy
 * - Integration with MongoDB for user authentication
 * - Utilization of Passport.js for handling user authentication
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Initial setup and configuration of Passport.js.
 */

const passport = require('passport');
const LocalStrategy = require('passport-local').Strategy;
const mongoose = require('mongoose');
const User = mongoose.model('users');

passport.use(new LocalStrategy({
    usernameField: 'email'
},
    (username, password, done) => {
        User.findOne({ email: username }, (err, user) => {
            if (err) { return done(err); }
            if (!user) {
                return done(null, false, {
                    message: 'Incorrect username.'
                });
            }
            if (!user.validPassword(password)) {
                return done(null, false, {
                    message: 'Incorrect password.'
                });
            }
            return done(null, user);
        });
    }
));