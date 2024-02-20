/*
 * Main Controller
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 01/27/2024
 * Version: 1.0
 *
 * Program Description:
 * This controller renders the main page of the application. It provides the foundational 
 * view that users first interact with upon visiting the site.
 *
 * Key Features:
 * - Rendering the main index view
 *
 * Revision History:
 * - Version 1.0 (01/27/2024): Setup to render the initial view of the application.
 */

const index = (req, res) => {
    res.render('index', { title: 'Travlr Getaways' });
}
module.exports = {
    index
}