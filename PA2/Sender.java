import java.io.*;
import java.net.*;

public class Sender {
    private static final int windowSize = 10;
    private static final int maxPacketLength = 500;
    private static final int SeqNumModulo = 32;
    private static final int timeoutLength = 100;

    private String emuAddress;
    private int emuRecvPort;
    private int senderRecvPort;
    private String fileName;

    private packet packets[];

    private PrintWriter seqWriter;
    private PrintWriter ackWriter;

    DatagramSocket clientSocket;

    long start_time;
    boolean timer_stopped = true;

    private int packetsSent = -1;
    private int packetsAcked = -1;

    public static void main(String[] args) throws Exception{
        if(args.length != 4){
            System.out.println("Please enter 4 input arguments!");
            System.exit(-1);
        }

        Sender sender = new Sender();
        sender.emuAddress = args[0];
        sender.emuRecvPort = Integer.parseInt(args[1]);
        sender.senderRecvPort = Integer.parseInt(args[2]);
        sender.fileName = args[3];

        /* create a Acknowledger thread */
        Thread ackThread = new Thread(sender.new Acknowledger());
        ackThread.start();

        sender.go();

        System.out.println("Finish main thread!");

    } // end main

    private void go() throws Exception {
        // System.out.println("In method go");
        clientSocket = new DatagramSocket();

        /* make a sequence number PrintWriter & an ack number PrintWriter to the corresponding log files */
        seqWriter = new PrintWriter("seqnum.log");
        ackWriter = new PrintWriter("ack.log");

        /* read bytes from file specified by filename into byte array fileBytes */
        byte[] fileBytes = readFile(fileName);

        /* make packets from fileBytes */
        packets = makePackets(fileBytes);

        /* send packets to the emulator until all packets have been acknowledged */
        while (packetsAcked < packets.length - 1) {
            // System.out.println("In main thread" + " current Thread: " + Thread.currentThread());
            System.out.println("packetsAcked = " + packetsAcked);
            System.out.println("packetsSent = " + packetsSent);

            int start = packetsSent + 1;
            int windowSizeLeft = windowSize - (packetsSent - packetsAcked);
            int numOfPacketsLeft = (packets.length - 1) - packetsSent;
            if (windowSizeLeft > 0 && numOfPacketsLeft > 0) {
                int length = Math.min(windowSizeLeft, numOfPacketsLeft);
                if (timer_stopped) {
                    start_time = System.currentTimeMillis();
                    timer_stopped = false;
                    System.out.println("start timer!");
                }
                sendPackets(packets, start, length);
            } else {
                /* The window is full. Try sending the packets later (let the main thread sleep for a while) */
            }
            // System.out.println("current Thread: " + Thread.currentThread());

            /* poll every 30 milliseconds to check whether timeout happened */
            Thread.sleep(30);
            long current_time = System.currentTimeMillis();
            long elapsed_time = current_time - start_time;
            // System.out.println("start time" + start_time);
            // System.out.println("current time" + current_time);
            System.out.println("elapsed time " + elapsed_time);
            if(elapsed_time >= timeoutLength  && timer_stopped == false) {
                System.out.println("time out!");
                timer_stopped = true;
                System.out.println("stop timer!");
                packetsSent = packetsAcked;
            }
        }

        /* send EOT packet after all packets have been ACKed. */
        packet EOTpacket = packet.createEOT(packets.length % SeqNumModulo);
        InetAddress IP = InetAddress.getByName(emuAddress);
        byte UDPdata[] = EOTpacket.getUDPdata();
        DatagramPacket UDPPacket = new DatagramPacket(UDPdata, UDPdata.length, IP, emuRecvPort);
        clientSocket.send(UDPPacket);
        System.out.println("EOT packet sent!");
    } // end go

    private byte[] readFile(String fileName){
        byte fileBytes[] = null;
        FileInputStream is;

        try{
            File file  = new File(fileName);
            fileBytes = new byte[(int)file.length()];
            is = new FileInputStream(file);
            is.read(fileBytes);
            is.close();
        } catch(FileNotFoundException ex){
            System.out.println("File not found!");
        } catch(IOException ex){
            ex.printStackTrace();
        }
        return fileBytes;
    } // end readFile

    private packet[] makePackets(byte[] fileBytes) throws Exception{
        int numOfPackets = (int)Math.ceil((double)fileBytes.length / maxPacketLength);
        packet packets[] = new packet[numOfPackets];

        /* make the first n - 1 packets */
        for(int i = 0; i < numOfPackets - 1; i++) {
            byte data[] = new byte[maxPacketLength];
            System.arraycopy(fileBytes, i * maxPacketLength, data, 0, maxPacketLength);
            packets[i] = packet.createPacket(i % SeqNumModulo, new String(data));
        }

        /* make the last packet */
        int lastPacketLength = fileBytes.length - (numOfPackets - 1) * maxPacketLength;
        byte data[] = new byte[lastPacketLength];
        System.arraycopy(fileBytes, (numOfPackets - 1) * maxPacketLength, data, 0, lastPacketLength);
        packets[numOfPackets-1] = packet.createPacket((numOfPackets-1) % SeqNumModulo, new String(data));

        return packets;
    } // end makePackets

    private void sendPackets(packet[] packets, int start, int length) throws Exception{
        InetAddress IP = InetAddress.getByName(emuAddress);

        for(int i = start; i < start + length; i++){
            byte UDPdata[] = packets[i].getUDPdata();
            DatagramPacket UDPPacket = new DatagramPacket(UDPdata, UDPdata.length, IP, emuRecvPort);
            clientSocket.send(UDPPacket);
            System.out.println("packet sent: " + packets[i].getSeqNum());
            seqWriter.println(packets[i].getSeqNum());
            seqWriter.flush();
        }
        packetsSent = start + length - 1;
    } // end send Packets

    public class Acknowledger implements Runnable{
        public void run() {
            try{
                receiveAcks();
            } catch(Exception ex){
                ex.printStackTrace();
            }
        } // end run

        public void receiveAcks() throws Exception{
            byte[] data = new byte[512];
            DatagramSocket ackSocket = new DatagramSocket(senderRecvPort);
            DatagramPacket UDPPacket = new DatagramPacket(data, data.length);
            packet recvPacket;
            // start_time = System.currentTimeMillis();

            /* allow the go() method in main thread some time to set up */
            Thread.sleep(50);

            while(true) {
                // System.out.println("In ack thread");

                /* receive ACK packets from receiver until EOT */
                ackSocket.receive(UDPPacket);
                recvPacket = packet.parseUDPdata(data);
                int seqNum = recvPacket.getSeqNum();
                int type = recvPacket.getType();
                System.out.println("ACK received: " + seqNum);

                if(type == 2){
                    // EOT
                    ackWriter.close();
                    seqWriter.close();
                    ackSocket.close();
                    clientSocket.close();
                    break;
                }

                ackWriter.println(seqNum);
                ackWriter.flush();

                if (seqNum == packetsAcked){
                    System.out.println(" Type: duplicate ACK");
                    /* server didn't receive packets in order. */
                    // do nothing, wait for timeout
                }else{
                    /* server received some packets in order, so we can move packetsAcked forward */
                    int offset = Math.floorMod((seqNum - packetsAcked), SeqNumModulo);
                    packetsAcked = packetsAcked + offset;
                    if(packetsAcked == packetsSent){
                        System.out.println(" Type: fully correct ACK");
                        // if all packets currently sent have been Acked, stop timer.
                        timer_stopped = true;
                        System.out.println("stop timer! (in ACK thread");
                    }else{
                        System.out.println(" Type: partly correct ACK");
                        // else, restart timer
                        start_time = System.currentTimeMillis();
                        System.out.println("restart timer! (in ACK thread");
                    }
                }

                /*
                if (seqNum == packetsAcked) {
                    System.out.println("Type: duplicate ACK");
                    /* server didn't receive packets in order.
                    // do nothing, wait for timeout
                } else if (seqNum == packetsSent) {
                    System.out.println("Type: fully correct ACK");
                    /* server received all packets in order. Stop the timer.
                    packetsAcked = seqNum;
                    timer_stopped = true;
                } else {
                    System.out.print("Type: partly correct ACK");
                    /* server received some but not all packets in order. Restart the timer.
                    packetsAcked = seqNum;
                    start_time = System.currentTimeMillis();
                }
                */
            }
            System.out.println("Finish Ack thread!");
        } // end receiveAcks
    } // end Acknowledger
}
