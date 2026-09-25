/*
 * Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

const WebSocket = require('ws');
const net = require('net');

const WEB_SOCKET_PORT = 5175; // the port connected by the frontend
const TCP_SERVER_PORT = 12345; // the tcp server port
const TCP_SERVER_HOST = '127.0.0.1'; // the tcp server host

// create a WebSocket server
const wss = new WebSocket.Server({ port: WEB_SOCKET_PORT }, () => {
  console.log(`WebSocket server started on port ${WEB_SOCKET_PORT}`);
});

wss.on('connection', (ws) => {
  console.log('New WebSocket client connected.');

  // establish a connection to the TCP server
  const tcpClient = new net.Socket();
  tcpClient.connect(TCP_SERVER_PORT, TCP_SERVER_HOST, () => {
    console.log('Connected to TCP server.');
  });

  // when the TCP server sends a message, forward it to the WebSocket client
  tcpClient.on('data', (data) => {
    console.log('Received from TCP server:', data.toString());
    ws.send(data.toString());
  });

  // when the WebSocket client sends a message, forward it to the TCP server
  ws.on('message', (message) => {
    console.log('Received from WebSocket client:', message);
    tcpClient.write(message);
  });

  ws.on('close', () => {
    console.log('WebSocket client disconnected.');
    tcpClient.end();
  });

  tcpClient.on('close', () => {
    console.log('TCP connection closed.');
    ws.close();
  });

  tcpClient.on('error', (err) => {
    console.error('TCP client error:', err);
    ws.close();
  });
});
