JC = javac
.SUFFIXES: .java .class
.java.class:
	$(JC) $*.java

CLASSES = \
        Sender.java \
        Receiver.java \
        packet.java \

default: classes

classes: $(CLASSES:.java=.class)

clean:
	$(RM) *.class