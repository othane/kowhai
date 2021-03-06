﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace kowhai_test
{
    class Sock : IComms
    {
        static IPAddress addr = IPAddress.Parse("127.0.0.1");
        static IPEndPoint ep = new IPEndPoint(addr, 55555);

        Socket sock;
        public event EventHandler Connected;
        public event EventHandler Disconnected;
        public event CommsReceiveEventHandler CommsReceived;

        public Sock()
        {
        }

        public bool Connect()
        {
            try
            {
                sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                sock.Connect(ep);
                DoConnected();
                return true;
            }
            catch (Exception e)
            {
                System.Diagnostics.Debug.WriteLine(e.Message);
                sock = null;
                return false;
            }
        }

        public void Disconnect()
        {
            if (sock != null)
            {
                sock.Disconnect(true);
                DoDisconnected();
            }
        }

        public int Send(byte[] buffer, int size)
        {
            if (sock == null)
                return -1;
            return sock.Send(buffer, size, SocketFlags.None);
        }

        public int Receive(byte[] buffer, int size)
        {
            return sock.Receive(buffer, size, SocketFlags.None);
        }

        struct StateObj
        {
            public byte[] Buffer;
            public int Size;
            public StateObj(byte[] buffer, int size)
            {
                this.Buffer = buffer;
                this.Size = size;
            }
        }

        void ReceiveCallback(IAsyncResult ar)
        {
            if (sock == null)
                return;
            try
            {
                StateObj state = (StateObj)ar.AsyncState;
                int read = sock.EndReceive(ar);
                if (read > 0)
                {
                    if (CommsReceived != null)
                        CommsReceived(this, new CommsReceiveEventArgs(state.Buffer, read));
                    sock.BeginReceive(state.Buffer, 0, state.Size, SocketFlags.None, new AsyncCallback(ReceiveCallback), state);
                }
                else
                    sock.Close();
            }
            catch (SocketException e)
            {
                if (e.ErrorCode == 10054) // Error code for Connection reset by peer
                {
                    sock.Close();
                    DoDisconnected();
                }
                else
                    throw e;
            }
        }

        public void StartAsyncReceives(byte[] buffer, int size)
        {
            StateObj state = new StateObj(buffer, size);
            sock.BeginReceive(buffer, 0, size, SocketFlags.None, new AsyncCallback(ReceiveCallback), state);
        }

        private void DoConnected()
        {
            if (Connected != null)
                Connected(this, new EventArgs());
        }

        private void DoDisconnected()
        {
            if (Disconnected != null)
                Disconnected(this, new EventArgs());
            sock = null;
        }

        public string GetErrorString()
        {
            return "Not implemented :(";
        }
    }
}
