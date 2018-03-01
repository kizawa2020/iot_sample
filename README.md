# IoT活用 サンプルコード(主にWioLTE/AWSIoT関連)
## 概要
本ソースは以下のURLに記載のWioLTE-AWSIoT連携向けにコードされたものです。
https://kizawa.info/wiolte-awsiot

## 一覧
|No.|ファイル名|概要|
|:---:|:---|:---|
|1|mqtt-client-shadowLED.ino|AWS IoTのデバイスシャドウのステータスに基づきLEDの色を変える(WioLTE)|
|2|mqtt-shadowLED-sensor.ino|1に加えて温度センサーの情報をSORACOM Beam経由でAWS IoTに送信する(WioLTE)|
|3|WioLTE-TempCheck.py|WioLTEから収集した温度を判定しAWS IoTのデバイスシャドウを変更するLambdaスクリプト(Python3.6)|

詳しくはブログ記事を参照ください。

## ブログURL
https://kizawa.info