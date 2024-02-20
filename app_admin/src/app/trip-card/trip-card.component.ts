import { Component, OnInit, Input } from '@angular/core';
import { Router } from '@angular/router';
import { Trip } from '../models/trip';
import { AuthenticationService } from '../services/authentication.service';
import { TripDataService } from '../services/trip-data.service';

@Component({
  selector: 'app-trip-card',
  templateUrl: './trip-card.component.html',
  styleUrls: ['./trip-card.component.css']
})
export class TripCardComponent implements OnInit {

  @Input('trip') trip: Trip;

  constructor(
    private router: Router,
    private authenticationService: AuthenticationService,
    private tripDataService: TripDataService
  ) { }

  ngOnInit(): void {
  }

  public isLoggedIn(): boolean {
    return this.authenticationService.isLoggedIn();
  }

  private editTrip(trip: Trip): void {
    console.log('Inside TripCardComponent#editTrip');
    localStorage.removeItem("tripCode");
    localStorage.setItem("tripCode", trip.code);
    this.router.navigate(['edit-trip']);
  }

  
  public deleteTrip(trip: Trip): void {
    console.log('Inside TripCardComponent#deleteTrip');
    // Confirm with the user before deletion
    if (confirm("Are you sure you want to delete this trip?")) {
      this.tripDataService.deleteTrip(trip.code)
        .then(() => {
          // Handle successful deletion, e.g., navigate away or update the view
          console.log('Trip deleted successfully');
          this.router.navigate(['']);
        })
        .catch(error => {
          console.error('Error deleting trip:', error);
        });
    }
  }
}

