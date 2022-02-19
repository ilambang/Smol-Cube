import json
import boto3
import logging
 
logger = logging.getLogger()
logger.setLevel(logging.INFO)
 
client = boto3.client('iot-data')

objekJOSn = json.loads('{"state": { "desired": { "status": "off", "status_TV": "off", "status_AC": "off"}}}')
 
def lambda_handler(event, context):
    print(json.dumps(event['body']))
    
    body = event['body']
    body = json.loads(body)
    
    THINGNAME=body['thingname']
    if (THINGNAME == ""):
        print("No Thing Name found. Setting Thing Name = ESP2")
        THINGNAME="ESP32"
    
    if body['action'] == "on":
        objekJOSn["state"]["desired"]["status"] = body['action']
        objekJOSn["state"]["desired"]["status_TV"] = body['action_TV']
        objekJOSn["state"]["desired"]["status_AC"] = body['action_AC']
        objekJOSn["state"]["desired"]["hex_TV_ON"] = body['hex_TV_ON']
        objekJOSn["state"]["desired"]["hex_TV_OFF"] = body['hex_TV_OFF']
        objekJOSn["state"]["desired"]["hex_AC_ON"] = body['hex_AC_ON']
        objekJOSn["state"]["desired"]["hex_AC_OFF"] = body['hex_AC_OFF']
        
        payload = json.dumps(objekJOSn) 
        # payload = json.dumps({'state': { 'desired': { 'status': 'on','status_TV': 'on' } }})
        
        logger.info("Attempting to Update Shadow State to ON")
        response = client.update_thing_shadow(
            thingName=THINGNAME,
            payload=payload
        )
        logger.info("IOT Shadow Updated")
    else:
        objekJOSn["state"]["desired"]["status"] = body['action']
        objekJOSn["state"]["desired"]["status_TV"] = body['action_TV']
        objekJOSn["state"]["desired"]["status_AC"] = body['action_AC']
        objekJOSn["state"]["desired"]["hex_TV_ON"] = body['hex_TV_ON']
        objekJOSn["state"]["desired"]["hex_TV_OFF"] = body['hex_TV_OFF']
        objekJOSn["state"]["desired"]["hex_AC_ON"] = body['hex_AC_ON']
        objekJOSn["state"]["desired"]["hex_AC_OFF"] = body['hex_AC_OFF']
        
        payload = json.dumps(objekJOSn) 
        # payload = json.dumps({'state': { 'desired': { 'status': 'off','status_TV': 'off' } }})
        
        logger.info("Attempting to Update Shadow State to OFF")
        response = client.update_thing_shadow(
            thingName=THINGNAME,
            payload=payload
        )
        logger.info("IOT Shadow Updated")
        
    
    return {
        'statusCode': 200,
        "headers": {
            'Access-Control-Allow-Origin':'*',
            'Access-Control-Allow-Headers':'Content-Type,X-Amz-Date,Authorization,X-Api-Key,X-Amz-Security-Token',
            'Access-Control-Allow-Methods':'GET,OPTIONS'
        },
        'body': json.dumps('Shadow Updated!')
    }
