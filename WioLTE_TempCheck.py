import boto3
import json
iot = boto3.client('iot-data')
thingName = '<device name>'
temp_threshold = 30

def lambda_handler(event, context):
    # eventを分解
    temp = event['temp']
    humi = event['humi']

    if temp > temp_threshold:
        ledcolor_desired = "red"
    else:
        ledcolor_desired = "off"
        
    shadow_stream = iot.get_thing_shadow(thingName = thingName)
    shadow_string = json.loads(shadow_stream['payload'].read().decode('utf-8'))
    ledcolor_status = shadow_string['state']['desired']['LED']

    if ledcolor_desired != ledcolor_status:
        # IoT シャドウを更新
        payload = {"state": {"desired": {"LED": ledcolor_desired }}}
        response = iot.update_thing_shadow(
            thingName = thingName,
            payload = json.dumps(payload)
        )

    return 'dedired={} status={} 温度：{}℃ 湿度：{}%'.format(ledcolor_desired,ledcolor_status,temp,humi)
