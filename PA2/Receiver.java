import java.io.*;
import java.net.*;

public class Receiver {
    private static final int SeqNumModule = 32;

    private String emuAddress;
    private int emuRecvPort;
    private int receiverRecvPort;
    private String fileName;

    private PrintWriter fileWriter;
    private PrintWriter seqWriter;

    private int expect = 0;

    public static void main(String[] args) throws Exception{
        if(args.length != 4){
            System.out.println("Please enter 4 input arguments!");
            System.exit(-1);
        }

        Receiver receiver = new Receiver();
        receiver.emuAddress = args[0];
        receiver.emuRecvPort = Integer.parseInt(args[1]);
        receiver.receiverRecvPort = Integer.parseInt(args[2]);
        receiver.fileName = args[3];
        receiver.fileWriter = new PrintWriter(receiver.fileName);
        receiver.seqWriter = new PrintWriter("arrival.log");

        receiver.go();
    } // end main

    public void go() throws Exception{
        InetAddress IP = InetAddress.getByName(emuAddress);
        DatagramSocket serverSocket = new DatagramSocket(receiverRecvPort);
        DatagramPacket UDPRecvPacket;
        packet recvPacket, ackPacket;

        while(true){
            byte[] recvData = new byte[512];
            UDPRecvPacket = new DatagramPacket(recvData, recvData.length);

            serverSocket.receive(UDPRecvPacket);
            recvPacket = packet.parseUDPdata(UDPRecvPacket.getData());
            int seqNum = recvPacket.getSeqNum();
            int type = recvPacket.getType();
            byte data[] = recvPacket.getData();

            if(type == 2){
                System.out.println("Sending back EOT ACK.");
                // EOT packet
                /* Send an EOT packet back and then exit. */
                ackPacket = packet.createEOT(seqNum);
                byte[] UDPdata = ackPacket.getUDPdata();
                DatagramPacket UDPPacket = new DatagramPacket(UDPdata, UDPdata.length, IP, emuRecvPort);
                // sendData = ackPacket.getData();
                // UDPSendPacket = new DatagramPacket(sendData, sendData.length, IP, emuRecvPort);
                serverSocket.send(UDPPacket);

                seqWriter.close();
                fileWriter.close();
                serverSocket.close();
                break;
            }

            else if(type == 1){
                // normal packets
                seqWriter.println(seqNum);
                seqWriter.flush();
                if(seqNum == expect) {
                    System.out.println("Sending back ACK for correct packet: " + seqNum);
                    /* Correct packet. Send an ACK for the received packet, print message to file,
                     and increase expect by 1(modulo 32). */
                    ackPacket = packet.createACK(seqNum);
                    byte[] UDPdata = ackPacket.getUDPdata();
                    DatagramPacket UDPPacket = new DatagramPacket(UDPdata, UDPdata.length, IP, emuRecvPort);
                    // sendData = ackPacket.getData();
                    // UDPSendPacket = new DatagramPacket(sendData, sendData.length, IP, emuRecvPort);
                    serverSocket.send(UDPPacket);

                    String message = new String(data);
                    // System.out.println(message);
                    fileWriter.print(message);
                    fileWriter.flush();
                    expect = (expect + 1) % SeqNumModule;
                }else{
                    /* if packet 0 got lost, don't ACK or log */
                    if(expect == 0){
                        continue;
                    }
                    System.out.println("Received: " + seqNum + ", sending back duplicate ACK: " + (expect - 1));
                    /* Wrong packet. Send an ACK for the most recently received correct packet,
                     and discard the currently received packet. */
                    ackPacket = packet.createACK((expect-1) % SeqNumModule);
                    byte[] UDPdata = ackPacket.getUDPdata();
                    DatagramPacket UDPPacket = new DatagramPacket(UDPdata, UDPdata.length, IP, emuRecvPort);
                    // sendData = ackPacket.getData();
                    // UDPSendPacket = new DatagramPacket(sendData, sendData.length, IP, emuRecvPort);
                    serverSocket.send(UDPPacket);
                }
            }
        }
    } // end go
}
