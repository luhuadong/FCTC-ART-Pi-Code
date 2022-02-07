const container = require('rhea');
const crypto = require('crypto');
//建立连接。
var connection = container.connect({
    'host': '${uid}.iot-amqp.${regionId}.aliyuncs.com',
    'port': 5671,
    'transport':'tls',
    'reconnect':true,
    'idle_time_out':60000,
    //按照阿里云IoT规范，组装UserName。其中的iotInstanceId，购买的实例请填写实例ID，公共实例请填空字符串""。
    'username':'${YourClientId}|authMode=aksign,signMethod=hmacsha1,timestamp=1573489088171,authId=${YourAccessKeyId},iotInstanceId=${YourIotInstanceId},consumerGroupId=${YourConsumerGroupId}|', 
    //按照阿里云IoT规范，计算签名，组装password。
    'password': hmacSha1('${YourAccessKeySecret}', 'authId=${YourAccessKeyID}&timestamp=1573489088171'),
});
//创建Receiver-Link。
var receiver = connection.open_receiver();
//接收云端推送消息的回调函数。
container.on('message', function (context) {
    var msg = context.message;
    var messageId = msg.message_id;
    var topic = msg.application_properties.topic;
    var content = Buffer.from(msg.body.content).toString();
    //发送ack，注意不要在回调函数有耗时逻辑。
    context.delivery.accept();
});
//计算password签名。
function hmacSha1(key, context) {
    return Buffer.from(crypto.createHmac('sha1', key).update(context).digest())
        .toString('base64');
}