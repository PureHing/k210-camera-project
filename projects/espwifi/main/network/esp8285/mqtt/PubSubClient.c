/*
  PubSubClient.cpp - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#include <string.h>
#include "PubSubClient.h"
#include "WiFiSpiClient.h"
#include "wifispi_drv.h"

#include "sysctl.h"
#include "myspi.h"

typedef struct _mqtt
{
    uint8_t sock;
    mqtt_cb cb;

    IPAddress_t ip;
    unsigned char domain[DOMAIN_MAX_LEN];
    uint8_t srv_type; //1 doamin, 0 ip
    uint16_t port;
    uint8_t retained;
    uint16_t keep_alive;

} mqtt_t;

static mqtt_t mqtt;
static int _state;
static uint16_t nextMsgId;
static unsigned long lastOutActivity;
static unsigned long lastInActivity;
static uint8_t pingOutstanding;

static uint8_t mqtt_buffer[MQTT_MAX_PACKET_SIZE];

static uint8_t readByte0(uint8_t *result);
static uint8_t readByte(uint8_t *result, uint16_t *index);
static uint16_t readPacket(uint8_t *lengthLength);
static size_t buildHeader(uint8_t header, uint8_t *buf, uint16_t length);
static uint8_t write(uint8_t header, uint8_t *buf, uint16_t length);
static void disconnect(void);
static uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos);

// PubSubClient::PubSubClient() {
//     this->_state = MQTT_DISCONNECTED;
//     this->_client = NULL;
//     this->stream = NULL;
//     setCallback(NULL);
// }

// PubSubClient::PubSubClient(Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setClient(client);
//     this->stream = NULL;
// }

// PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(addr, port);
//     setClient(client);
//     this->stream = NULL;
// }
// PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(addr,port);
//     setClient(client);
//     setStream(stream);
// }
// PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(addr, port);
//     setCallback(callback);
//     setClient(client);
//     this->stream = NULL;
// }
// PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(addr,port);
//     setCallback(callback);
//     setClient(client);
//     setStream(stream);
// }

// PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(ip, port);
//     setClient(client);
//     this->stream = NULL;
// }
// PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(ip,port);
//     setClient(client);
//     setStream(stream);
// }
// PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(ip, port);
//     setCallback(callback);
//     setClient(client);
//     this->stream = NULL;
// }
// PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(ip,port);
//     setCallback(callback);
//     setClient(client);
//     setStream(stream);
// }

// PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(domain,port);
//     setClient(client);
//     this->stream = NULL;
// }
// PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(domain,port);
//     setClient(client);
//     setStream(stream);
// }

void PubSubClient_init(const char *domain, uint8_t *ip, uint16_t port, uint8_t retained, uint16_t keep_alive, mqtt_cb callback)
{
    _state = MQTT_DISCONNECTED;
    if (domain)
    {
        //domain
        uint16_t d_len = strlen(domain);
        memcpy(mqtt.domain, domain, d_len);
        mqtt.domain[d_len] = '\0';
        mqtt.srv_type = 1;
    }
    else if (ip)
    {
        memcpy(mqtt.ip.address, ip, WL_IPV4_LENGTH);
        mqtt.srv_type = 0;
    }
    else
    {
        printk("server must set, by doamin ot ipaddress\r\n");
    }

    mqtt.port = port;
    mqtt.cb = callback;
    mqtt.retained = retained;
    mqtt.keep_alive = keep_alive;

    return;
}

int PubSubClient_state(void)
{
    return _state;
}

// PubSubClient::PubSubClient(const char* domain, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
//     this->_state = MQTT_DISCONNECTED;
//     setServer(domain,port);
//     setCallback(callback);
//     setClient(client);
//     setStream(stream);
// }

uint8_t PubSubClient_connect0(const char *id)
{
    return PubSubClient_connect(id, NULL, NULL, NULL, 0, 0, 0, 1);
}

uint8_t PubSubClient_connect1(const char *id, const char *user, const char *pass)
{
    return PubSubClient_connect(id, user, pass, 0, 0, 0, 0, 1);
}

uint8_t PubSubClient_connect2(const char *id, const char *willTopic, uint8_t willQos, uint8_t willRetain, const char *willMessage)
{
    return PubSubClient_connect(id, NULL, NULL, willTopic, willQos, willRetain, willMessage, 1);
}

uint8_t PubSubClient_connect3(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, uint8_t willRetain, const char *willMessage)
{
    return PubSubClient_connect(id, user, pass, willTopic, willQos, willRetain, willMessage, 1);
}

uint8_t PubSubClient_connect(const char *id, const char *user, const char *pass,
                             const char *willTopic, uint8_t willQos, uint8_t willRetain,
                             const char *willMessage, uint8_t cleanSession)
{
    if (!PubSubClient_connected())
    {
        uint8_t sock;

        if (mqtt.srv_type)
        {
            sock = WiFiSpiClient_connect_host(mqtt.domain, mqtt.port, 0);
        }
        else
        {
            sock = WiFiSpiClient_connect_ip(mqtt.ip, mqtt.port, 0);
        }

        if (sock != SOCK_NOT_AVAIL)
        {
            mqtt.sock = sock;

            nextMsgId = 1;
            // Leave room in the mqtt_buffer for header and variable length field
            uint16_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
            uint8_t d[9] = {0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
            uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 7
#endif
            for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++)
            {
                mqtt_buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic)
            {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else
            {
                v = 0x00;
            }
            if (cleanSession)
            {
                v = v | 0x02;
            }

            if (user != NULL)
            {
                v = v | 0x80;

                if (pass != NULL)
                {
                    v = v | (0x80 >> 1);
                }
            }

            mqtt_buffer[length++] = v;

            mqtt_buffer[length++] = ((mqtt.keep_alive) >> 8);
            mqtt_buffer[length++] = ((mqtt.keep_alive) & 0xFF);

            CHECK_STRING_LENGTH(length, id)
            length = writeString(id, mqtt_buffer, length);
            if (willTopic)
            {
                CHECK_STRING_LENGTH(length, willTopic)
                length = writeString(willTopic, mqtt_buffer, length);
                CHECK_STRING_LENGTH(length, willMessage)
                length = writeString(willMessage, mqtt_buffer, length);
            }

            if (user != NULL)
            {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, mqtt_buffer, length);
                if (pass != NULL)
                {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, mqtt_buffer, length);
                }
            }

            write(MQTTCONNECT, mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);

            lastInActivity = lastOutActivity = get_millis();

            while (WiFiSpiClient_available(mqtt.sock) == 0)
            {
                unsigned long t = get_millis();
                if (t - lastInActivity >= ((int32_t)MQTT_SOCKET_TIMEOUT * 1000UL))
                {
                    _state = MQTT_CONNECTION_TIMEOUT;
                    WiFiSpiClient_stop(mqtt.sock);
                    return 0;
                }
            }

            uint8_t llen;
            uint16_t len = readPacket(&llen);

            if (len == 4)
            {
                if (mqtt_buffer[3] == 0)
                {
                    lastInActivity = get_millis();
                    pingOutstanding = 0;
                    _state = MQTT_CONNECTED;
                    return 1;
                }
                else
                {
                    _state = mqtt_buffer[3];
                }
            }
            // _client->stop();
            WiFiSpiClient_stop(mqtt.sock);
        }
        else
        {
            _state = MQTT_CONNECT_FAILED;
        }
        return 0;
    }
    return 1;
}

// reads a byte into result
static uint8_t readByte0(uint8_t *result)
{
    uint32_t previousMillis = get_millis();
    while (!WiFiSpiClient_available(mqtt.sock))
    {
        // yield();
        uint32_t currentMillis = get_millis();
        if (currentMillis - previousMillis >= ((int32_t)MQTT_SOCKET_TIMEOUT * 1000))
        {
            return false;
        }
    }
    // *result = _client->read();
    *result = WiFiSpiClient_read_oneByte(mqtt.sock);
    return true;
}

// reads a byte into result[*index] and increments index
static uint8_t readByte(uint8_t *result, uint16_t *index)
{
    uint16_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    if (readByte0(write_address))
    {
        *index = current_index + 1;
        return true;
    }
    return false;
}

static uint16_t readPacket(uint8_t *lengthLength)
{
    uint16_t len = 0;
    if (!readByte(mqtt_buffer, &len))
        return 0;
    bool isPublish = (mqtt_buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint16_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint8_t start = 0;

    do
    {
        if (len == 5)
        {
            // Invalid remaining length encoding - kill the connection
            _state = MQTT_DISCONNECTED;
            // _client->stop();
            WiFiSpiClient_stop(mqtt.sock);
            return 0;
        }
        if (!readByte0(&digit))
            return 0;
        mqtt_buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish)
    {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if (!readByte(mqtt_buffer, &len))
            return 0;
        if (!readByte(mqtt_buffer, &len))
            return 0;
        skip = (mqtt_buffer[*lengthLength + 1] << 8) + mqtt_buffer[*lengthLength + 2];
        start = 2;
        if (mqtt_buffer[0] & MQTTQOS1)
        {
            // skip message id
            skip += 2;
        }
    }

    for (uint16_t i = start; i < length; i++)
    {
        if (!readByte0(&digit))
            return 0;
        // if (this->stream) {
        //     if (isPublish && len-*lengthLength-2>skip) {
        //         this->stream->write(digit);
        //     }
        // }
        if (len < MQTT_MAX_PACKET_SIZE)
        {
            mqtt_buffer[len] = digit;
        }
        len++;
    }

    // if (!this->stream && len > MQTT_MAX_PACKET_SIZE) {
    //     len = 0; // This will cause the packet to be ignored.
    // }

    if (len > MQTT_MAX_PACKET_SIZE)
    {
        len = 0; // This will cause the packet to be ignored.
    }

    return len;
}

#define POOL_INTERVAL       (100*1000)        //100ms

static uint64_t last_pool_tm = 0;

uint8_t PubSubClient_loop(void)
{
    uint64_t tm = sysctl_get_time_us();
    if (tm - last_pool_tm < POOL_INTERVAL)
    {
        return false;
    }
    last_pool_tm = tm;
    
    if (PubSubClient_connected())
    {
        unsigned long t = get_millis();
        if ((t - lastInActivity > mqtt.keep_alive * 1000UL) || (t - lastOutActivity > mqtt.keep_alive * 1000UL))
        {
            if (pingOutstanding)
            {
                _state = MQTT_CONNECTION_TIMEOUT;
                // _client->stop();
                WiFiSpiClient_stop(mqtt.sock);
                return false;
            }
            else
            {
                mqtt_buffer[0] = MQTTPINGREQ;
                mqtt_buffer[1] = 0;
                // _client->write(mqtt_buffer, 2);
                WiFiSpiClient_write(mqtt.sock, mqtt_buffer, 2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (WiFiSpiClient_available(mqtt.sock))
        {
            uint8_t llen;
            uint16_t len = readPacket(&llen);
            uint16_t msgId = 0;
            uint8_t *payload;
            if (len > 0)
            {
                lastInActivity = t;
                uint8_t type = mqtt_buffer[0] & 0xF0;
                if (type == MQTTPUBLISH)
                {
                    if (mqtt.cb)
                    {
                        uint16_t tl = (mqtt_buffer[llen + 1] << 8) + mqtt_buffer[llen + 2]; /* topic length in bytes */
                        memmove(mqtt_buffer + llen + 2, mqtt_buffer + llen + 3, tl);        /* move topic inside mqtt_buffer 1 byte to front */
                        mqtt_buffer[llen + 2 + tl] = 0;                                     /* end the topic as a 'C' string with \x00 */
                        char *topic = (char *)mqtt_buffer + llen + 2;
                        // msgId only present for QOS>0
                        if ((mqtt_buffer[0] & 0x06) == MQTTQOS1)
                        {
                            msgId = (mqtt_buffer[llen + 3 + tl] << 8) + mqtt_buffer[llen + 3 + tl + 1];
                            payload = mqtt_buffer + llen + 3 + tl + 2;
                            mqtt.cb(topic, msgId, payload, len - llen - 3 - tl - 2);

                            mqtt_buffer[0] = MQTTPUBACK;
                            mqtt_buffer[1] = 2;
                            mqtt_buffer[2] = (msgId >> 8);
                            mqtt_buffer[3] = (msgId & 0xFF);
                            // _client->write(mqtt_buffer, 4);
                            WiFiSpiClient_write(mqtt.sock, mqtt_buffer, 4);
                            lastOutActivity = t;
                        }
                        else
                        {
                            payload = mqtt_buffer + llen + 3 + tl;
                            mqtt.cb(topic, 0x0, payload, len - llen - 3 - tl);
                        }
                        memset(mqtt_buffer,0,sizeof(mqtt_buffer));
                    }
                }
                else if (type == MQTTPINGREQ)
                {
                    mqtt_buffer[0] = MQTTPINGRESP;
                    mqtt_buffer[1] = 0;
                    // _client->write(mqtt_buffer, 2);
                    WiFiSpiClient_write(mqtt.sock, mqtt_buffer, 2);
                }
                else if (type == MQTTPINGRESP)
                {
                    pingOutstanding = false;
                }
            }
            else if (!PubSubClient_connected())
            {
                // readPacket has closed the connection
                return false;
            }
        }
        return true;
    }
    return false;
}

// boolean PubSubClient::publish(const char* topic, const char* payload) {
//     return publish(topic,(const uint8_t*)payload,strlen(payload),false);
// }

// boolean PubSubClient::publish(const char* topic, const char* payload, boolean retained) {
//     return publish(topic,(const uint8_t*)payload,strlen(payload),retained);
// }

// boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength) {
//     return publish(topic, payload, plength, false);
// }

uint8_t PubSubClient_publish(const char *topic, const uint8_t *payload, unsigned int plength)
{
    if (PubSubClient_connected())
    {
        if (MQTT_MAX_PACKET_SIZE < MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + plength)
        {
            // Too long
            return false;
        }
        // Leave room in the mqtt_buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, mqtt_buffer, length);
        uint16_t i;
        for (i = 0; i < plength; i++)
        {
            mqtt_buffer[length++] = payload[i];
        }
        uint8_t header = MQTTPUBLISH;
        if (mqtt.retained)
        {
            header |= 1;
        }
        return write(header, mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

// boolean PubSubClient::publish_P(const char* topic, const char* payload, boolean retained) {
//     return publish_P(topic, (const uint8_t*)payload, strlen(payload), retained);
// }

// uint8_t PubSubClient_publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
// {
//     uint8_t llen = 0;
//     uint8_t digit;
//     unsigned int rc = 0;
//     uint16_t tlen;
//     unsigned int pos = 0;
//     unsigned int i;
//     uint8_t header;
//     unsigned int len;

//     if (!PubSubClient_connected())
//     {
//         return false;
//     }

//     tlen = strlen(topic);

//     header = MQTTPUBLISH;
//     if (retained)
//     {
//         header |= 1;
//     }
//     mqtt_buffer[pos++] = header;
//     len = plength + 2 + tlen;
//     do
//     {
//         digit = len % 128;
//         len = len / 128;
//         if (len > 0)
//         {
//             digit |= 0x80;
//         }
//         mqtt_buffer[pos++] = digit;
//         llen++;
//     } while (len > 0);

//     pos = writeString(topic, mqtt_buffer, pos);

//     rc += _client->write(mqtt_buffer, pos);

//     for (i = 0; i < plength; i++)
//     {
//         rc += _client->write((char)pgm_read_byte_near(payload + i));
//     }

//     lastOutActivity = millis();

//     return rc == tlen + 4 + plength;
// }

// boolean PubSubClient_beginPublish(const char *topic, unsigned int plength, boolean retained)
// {
//     if (PubSubClient_connected())
//     {
//         // Send the header and variable length field
//         uint16_t length = MQTT_MAX_HEADER_SIZE;
//         length = writeString(topic, mqtt_buffer, length);
//         uint16_t i;
//         uint8_t header = MQTTPUBLISH;
//         if (retained)
//         {
//             header |= 1;
//         }
//         size_t hlen = buildHeader(header, mqtt_buffer, plength + length - MQTT_MAX_HEADER_SIZE);
//         uint16_t rc = _client->write(mqtt_buffer + (MQTT_MAX_HEADER_SIZE - hlen), length - (MQTT_MAX_HEADER_SIZE - hlen));
//         lastOutActivity = millis();
//         return (rc == (length - (MQTT_MAX_HEADER_SIZE - hlen)));
//     }
//     return false;
// }

// uint8_t PubSubClient_endPublish()
// {
//     return 1;
// }

static size_t buildHeader(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do
    {
        digit = len % 128;
        len = len / 128;
        if (len > 0)
        {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++)
    {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

// size_t PubSubClient::write(uint8_t data) {
//     lastOutActivity = millis();
//     return _client->write(data);
// }

// size_t PubSubClient_write(const uint8_t *mqtt_buffer, size_t size)
// {
//     lastOutActivity = millis();
//     return _client->write(mqtt_buffer, size);
// }

static uint8_t write(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE
    uint8_t *writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
    uint16_t bytesRemaining = length + hlen; //Match the length type
    uint8_t bytesToWrite;
    boolean result = true;
    while ((bytesRemaining > 0) && result)
    {
        bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE) ? MQTT_MAX_TRANSFER_SIZE : bytesRemaining;
        // rc = _client->write(writeBuf, bytesToWrite);
        rc = WiFiSpiClient_write(mqtt.sock, writeBuf, bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    return result;
#else
    // rc = _client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    rc = WiFiSpiClient_write(mqtt.sock, buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);

    lastOutActivity = get_millis();
    return (rc == hlen + length) ? 1 : 0;
#endif
}

// boolean PubSubClient::subscribe(const char* topic) {
//     return subscribe(topic, 0);
// }

uint8_t PubSubClient_subscribe(const char *topic, uint8_t qos)
{
    if (qos > 1)
    {
        return false;
    }
    if (MQTT_MAX_PACKET_SIZE < 9 + strlen(topic))
    {
        // Too long
        return false;
    }
    if (PubSubClient_connected())
    {
        // Leave room in the mqtt_buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        mqtt_buffer[length++] = (nextMsgId >> 8);
        mqtt_buffer[length++] = (nextMsgId & 0xFF);
        length = writeString((char *)topic, mqtt_buffer, length);
        mqtt_buffer[length++] = qos;
        return write(MQTTSUBSCRIBE | MQTTQOS1, mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

uint8_t PubSubClient_unsubscribe(const char *topic)
{
    if (MQTT_MAX_PACKET_SIZE < 9 + strlen(topic))
    {
        // Too long
        return false;
    }
    if (PubSubClient_connected())
    {
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        mqtt_buffer[length++] = (nextMsgId >> 8);
        mqtt_buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, mqtt_buffer, length);
        return write(MQTTUNSUBSCRIBE | MQTTQOS1, mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

static void disconnect(void)
{
    mqtt_buffer[0] = MQTTDISCONNECT;
    mqtt_buffer[1] = 0;
    WiFiSpiClient_write(mqtt.sock, mqtt_buffer, 2);
    _state = MQTT_DISCONNECTED;
    WiFiSpiClient_flush(mqtt.sock);
    WiFiSpiClient_stop(mqtt.sock);
    lastInActivity = lastOutActivity = get_millis();
}

static uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos)
{
    const char *idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp)
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

uint8_t PubSubClient_connected(void)
{
    uint8_t rc;
    // if (_client == NULL ) {
    //     rc = false;
    // } else
    {
        rc = WiFiSpiClient_connected(mqtt.sock);
        if (!rc)
        {
            if (_state == MQTT_CONNECTED)
            {
                _state = MQTT_CONNECTION_LOST;
                WiFiSpiClient_flush(mqtt.sock);
                WiFiSpiClient_stop(mqtt.sock);
            }
        }
    }
    return rc;
}

// PubSubClient &PubSubClient::setServer(uint8_t *ip, uint16_t port)
// {
//     IPAddress addr(ip[0], ip[1], ip[2], ip[3]);
//     return setServer(addr, port);
// }

// void PubSubClient_setServer_ip(IPAddress_t ip, uint16_t port)
// {
//     // this->ip = ip;
//     // this->port = port;
//     // this->domain = NULL;

//     memcpy(mqtt_ip.address, ip.address, WL_IPV4_LENGTH);
//     mqtt_port = port;
//     mqtt_domain = NULL;
//     return;
// }

// void PubSubClient_setServer_host(const char *domain, uint16_t port)
// {
//     // this->domain = domain;
//     // this->port = port;

//     mqtt_domain = domain;
//     mqtt_port = port;
//     return;
// }

// PubSubClient &PubSubClient::setCallback(MQTT_CALLBACK_SIGNATURE)
// {
//     this->callback = callback;
//     return *this;
// }

// PubSubClient& PubSubClient::setClient(Client& client){
//     this->_client = &client;
//     return *this;
// }

// PubSubClient& PubSubClient::setStream(Stream& stream){
//     this->stream = &stream;
//     return *this;
// }

// int PubSubClient::state()
// {
//     return this->_state;
// }
