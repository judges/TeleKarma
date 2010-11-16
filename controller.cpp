/*
 * controller.cpp
 *
 * Implementation of the controller super class.
 *
 */
 #include "controller.h"
 #include "model.h"
 
 
 //Only constructor for the Controller class?
 Controller::Controller(Model * model) : model(model) {
	 
 }
 
 Controller::~Controller() { 
	//Don't do anything in here
 }