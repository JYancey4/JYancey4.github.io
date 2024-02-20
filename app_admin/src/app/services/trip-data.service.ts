import { Injectable, Inject } from '@angular/core';
import { Http, Headers, RequestOptions } from '@angular/http';

import { Trip } from '../models/trip';
import { User } from '../models/user';
import { AuthResponse } from '../models/authresponse';
import { BROWSER_STORAGE } from '../storage';

@Injectable()
export class TripDataService {
  constructor(private http: Http,
  @Inject(BROWSER_STORAGE) private storage: Storage) { }


  private apiBaseUrl = 'http://localhost:3000/api/';
  private tripUrl = `${this.apiBaseUrl}trips/`;

  public addTrip(formData: Trip): Promise<Trip> {
    console.log('Inside TripDataService#addTrip');

    let headers = new Headers({ 'Authorization': `Bearer ${this.getToken()}` });
    let options = new RequestOptions({ headers: headers });

    return this.http
      .post(this.tripUrl, formData, options)
      .toPromise()
      .then(response => response.json() as Trip)
      .catch(error => {
        console.error('Error adding trip:', error);
        return this.handleError(error);
      });
  }

  public getTrip(tripCode: string): Promise<Trip> {
    console.log('Inside TripDataService#getTrip(tripCode)');
    return this.http
      .get(this.tripUrl + tripCode)
      .toPromise()
      .then(response => response.json() as Trip)
      .catch(this.handleError);
  }


  public getTrips(): Promise<Trip[]> {
    console.log('Inside TripDataService#getTrips');
    return this.http
      .get(this.tripUrl)
      .toPromise()
      .then(response => response.json() as Trip)
      .catch(error => {
        console.error('Error fetching trips:', error);
        return this.handleError(error);
      });
  }

  public updateTrip(formData: Trip): Promise<Trip> {
    console.log('Inside TripDataService#upateTrip');
    console.log(formData);

    let headers = new Headers({ 'Authorization': `Bearer ${this.getToken()}` });
    let options = new RequestOptions({ headers: headers });

    return this.http
      .put(this.tripUrl + formData.code, formData, options)
      .toPromise()
      .then(response => response.json() as Trip)
      .catch(this.handleError);
  }

  
  public deleteTrip(tripCode: string): Promise<void> {
    console.log('Inside TripDataService#deleteTrip');
    const url = `${this.tripUrl}${tripCode}`;

    let headers = new Headers({ 'Authorization': `Bearer ${this.getToken()}` });
    let options = new RequestOptions({ headers: headers });

    return this.http
      .delete(url, options)
      .toPromise()
      .then(() => null)
      .catch(this.handleError);
  }

  // Helper method to get the token from storage
  private getToken(): string {
    return this.storage.getItem('travlr-token');
  }

  private handleError(error: any): Promise<any> {
    console.error('An error occurred:', error);

    // Log the HTTP status code if available
    if (error.status) {
      console.error('HTTP status code:', error.status);
    }

    return Promise.reject(error.message || error);
  }

  public login(user: User): Promise<AuthResponse> {
    return this.makeAuthApiCall('login', user);
  }
  public register(user: User): Promise<AuthResponse> {
    return this.makeAuthApiCall('register', user);
  }
  private makeAuthApiCall(urlPath: string, user: User):
    Promise<AuthResponse> {
    const url: string = `${this.apiBaseUrl}/${urlPath}`;
    return this.http
      .post(url, user)
      .toPromise()
      .then(response => response.json() as AuthResponse)
      .catch(this.handleError);
  }
}
