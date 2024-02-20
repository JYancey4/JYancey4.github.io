
import { Component, OnInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from "@angular/forms";
import { Router } from "@angular/router";
import { TripDataService } from '../services/trip-data.service';

@Component({
  selector: 'app-delete-trip',
  templateUrl: './delete-trip.component.html',
  styleUrls: ['./delete-trip.component.css']
})
export class DeleteTripComponent implements OnInit {

  deleteForm: FormGroup;
  submitted = false;

  constructor(
    private formBuilder: FormBuilder,
    private router: Router,
    private tripService: TripDataService
  ) { }

  ngOnInit() {
    this.deleteForm = this.formBuilder.group({
      code: ['', Validators.required],
    });
  }

  onSubmit() {
    this.submitted = true;
    if (this.deleteForm.valid) {
      this.tripService.deleteTrip(this.deleteForm.value.code)
        .then(() => {
          console.log("Trip deleted successfully");
          this.router.navigate(['']);
        })
        .catch(error => console.error('Error in deleteTrip:', error));
    }
  }

  get f() { return this.deleteForm.controls; }
}

