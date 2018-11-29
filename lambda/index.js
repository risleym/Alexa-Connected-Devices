/**
 *   index.js
 *   
 *   Lambda Node.JS application for Alexa Connected Devices Training
 *   
 *   Prototyping Team 2018 Lab126/Amazon
 *   Michael Risley
 *   
 *   Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *   Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance with the License. A copy of the License is located at
 *       http://aws.amazon.com/apache2.0/
 *   or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */

/*
 * App ID for your FeatherSkill
 */
var APP_ID = "TODO"; //TODO: Enter your skill ID here

/*
 * AWS IoT Data 
 */
var ledTopic = "arduino/control";
var config = {};
config.IOT_BROKER_ENDPOINT      = "TODO".toLowerCase(); //TODO: Enter your IoT address here
config.IOT_BROKER_REGION        = "us-east-1";
config.IOT_THING_NAME           = "arduino";

/*
 * Load AWS SDK libraries
 */
var AWS = require('aws-sdk');
AWS.config.region = config.IOT_BROKER_REGION;
var AlexaSkill = require('./AlexaSkill');
var SKILL = function () {
    AlexaSkill.call(this, APP_ID);
};

/* Initialize client for IoT */
var iotData = new AWS.IotData({endpoint: config.IOT_BROKER_ENDPOINT});


/* Global state of the Feather's physical switch */
var switch_state = "unknown";

/* Create AlexaSkill Object */
SKILL.prototype = Object.create(AlexaSkill.prototype);
SKILL.prototype.constructor = SKILL;
SKILL.prototype.eventHandlers.onSessionStarted = function (sessionStartedRequest, session) {
    console.log("HelloWorld onSessionStarted requestId: " + sessionStartedRequest.requestId
        + ", sessionId: " + session.sessionId);
    // Any initialization logic goes here
};

SKILL.prototype.eventHandlers.onLaunch = function (launchRequest, session, response) {
    console.log("HelloWorld onLaunch requestId: " + launchRequest.requestId + ", sessionId: " + session.sessionId);
    var speechOutput = "Welcome to Alexa Skills Kit, you can use this skill to interact with your Wicked Feather";
    var repromptText = "You can say ... Turn the LED on, or ask for the state of the switch";
    response.ask(speechOutput, repromptText);
};

SKILL.prototype.eventHandlers.onSessionEnded = function (sessionEndedRequest, session) {
    console.log("HelloWorld onSessionEnded requestId: " + sessionEndedRequest.requestId
        + ", sessionId: " + session.sessionId);
    // Any cleanup logic goes here
};

SKILL.prototype.intentHandlers = {
    
    // Custom Intent Handler for the LED State
    "ledIntent": function (intent, session, response) {
        var repromptText = null;
        var sessionAttributes = {};
        var shouldEndSession = true;
        var speechOutput = "";
        var payloadObj;
        
        // Get the LIST_OF_STATES
        var stateSlot = intent.slots.STATE.value;
        console.log("my state was: " + stateSlot);
        
        // Are we turning it On or Off?
        if (stateSlot.toLowerCase() == "off")
            payloadObj=1; 
        else if( stateSlot.toLowerCase() == "on")
            payloadObj=0;
        else
            response.tell("Haha! It has to be ON or OFF");
            
        // Prepare the parameters to send to AWS IoT
        var paramsUpdate = {
            topic:ledTopic,
            payload: JSON.stringify(payloadObj),
            qos:0
        };
        // Send new LED value to AWS IoT and update Alexa's response
        iotData.publish(paramsUpdate, function(err, data) {
          if (err){
            //Handle the error here
            console.log("MQTT Error" + data);
          }
          else {
            // Alexa's response
            speechOutput = "Turning the LED "+stateSlot;
            console.log(data);
            response.tell(speechOutput);
          }    
        });
    },
    // Custom Intent Handler for the Switch State
   "stateIntent": function (intent, session, response) {
        var repromptText = null;
        var sessionAttributes = {};
        var shouldEndSession = true;
        var speechOutput = "";
        var payloadObj;
        response.tell("the switch is currently "+switch_state);
    },
    // EXAMPLE for creating a new intent handler
     "IfYouCreateAnotherIntent": function (intent, session, response) {
        console.log("FB started");
        var repromptText = null;
        var sessionAttributes = {};
        var shouldEndSession = true;
        var speechOutput = "";
        // Your code here
    },
    "AMAZON.HelpIntent": function (intent, session, response) {
        response.ask("You can ask me to turn the LED on or off, or for the state of the switch.");
    }
};

/*
 * Create the handler that responds to the Alexa Request 
 */
exports.handler = function (event, context) {
    
    // Console logs will appear in your CloudWatch (aws.amazon.com, search for cloudwatch)
    console.log("=====START=====");
    
        // Determine if this was ASK or IoT that triggered Lambda
        if(event.version==null)
        {    
            var eventText = JSON.stringify(event, null, 2);
            console.log("AWS IoT Received event:", eventText);
            
            // Store new state of the physical switch
            switch_state = event.state;
        }
        else
        {
            // Create an instance for Alexa response
            var skill = new SKILL();
            skill.execute(event, context);
        }
};
