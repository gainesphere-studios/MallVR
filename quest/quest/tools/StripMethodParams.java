import java.io.*;
import java.nio.file.*;
import java.util.stream.*;

/* MallVR Quest build tool: removes the MethodParameters attribute (parameter names for reflection, nothing else uses it) from every
   .class file under a folder. d8 from build-tools 34 crashes on the nameless entries newer Apache Commons Compress jars carry
   ("Cannot invoke String.length() because <parameter1> is null"). Run on an UNPACKED copy of the jar:
     java StripMethodParams.java <folder> */
public class StripMethodParams {
    public static void main(String[] a) throws IOException {
        int[] n = { 0, 0 };
        try (Stream<Path> s = Files.walk(Paths.get(a[0]))) {
            for (Path p : s.filter(x -> x.toString().endsWith(".class")).collect(Collectors.toList())) {
                byte[] in = Files.readAllBytes(p); int[] dropped = { 0 };
                byte[] out = strip(in, dropped);
                if (dropped[0] > 0) { Files.write(p, out); n[0]++; n[1] += dropped[0]; }
            }
        }
        System.out.println("MethodParameters removed: " + n[1] + " in " + n[0] + " classes");
    }

    static byte[] strip(byte[] b, int[] dropped) throws IOException {
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(b));
        ByteArrayOutputStream bo = new ByteArrayOutputStream(); DataOutputStream o = new DataOutputStream(bo);
        o.writeInt(in.readInt()); o.writeShort(in.readUnsignedShort()); o.writeShort(in.readUnsignedShort());   // magic, minor, major
        int cpn = in.readUnsignedShort(); o.writeShort(cpn);
        String[] utf = new String[cpn];
        for (int i = 1; i < cpn; i++) {
            int tag = in.readUnsignedByte(); o.writeByte(tag);
            switch (tag) {
                case 1: { String s = in.readUTF(); utf[i] = s; o.writeUTF(s); break; }
                case 3: case 4: o.writeInt(in.readInt()); break;
                case 5: case 6: o.writeLong(in.readLong()); i++; break;
                case 7: case 8: case 16: case 19: case 20: o.writeShort(in.readUnsignedShort()); break;
                case 9: case 10: case 11: case 12: case 17: case 18: o.writeInt(in.readInt()); break;
                case 15: o.writeByte(in.readUnsignedByte()); o.writeShort(in.readUnsignedShort()); break;
                default: throw new IOException("constant pool tag " + tag);
            }
        }
        o.writeShort(in.readUnsignedShort()); o.writeShort(in.readUnsignedShort()); o.writeShort(in.readUnsignedShort());  // access, this, super
        int ni = in.readUnsignedShort(); o.writeShort(ni); for (int i = 0; i < ni; i++) o.writeShort(in.readUnsignedShort());
        for (int pass = 0; pass < 2; pass++) {             // fields, then methods
            int nm = in.readUnsignedShort(); o.writeShort(nm);
            for (int m = 0; m < nm; m++) {
                o.writeShort(in.readUnsignedShort()); o.writeShort(in.readUnsignedShort()); o.writeShort(in.readUnsignedShort());
                attrs(in, o, utf, pass == 1, dropped);
            }
        }
        attrs(in, o, utf, false, dropped);                 // class attributes
        return bo.toByteArray();
    }

    static void attrs(DataInputStream in, DataOutputStream o, String[] utf, boolean drop, int[] dropped) throws IOException {
        int na = in.readUnsignedShort();
        ByteArrayOutputStream keep = new ByteArrayOutputStream(); DataOutputStream k = new DataOutputStream(keep); int kept = 0;
        for (int i = 0; i < na; i++) {
            int name = in.readUnsignedShort(); int len = in.readInt(); byte[] body = new byte[len]; in.readFully(body);
            if (drop && "MethodParameters".equals(utf[name])) { dropped[0]++; continue; }
            k.writeShort(name); k.writeInt(len); k.write(body); kept++;
        }
        o.writeShort(kept); o.write(keep.toByteArray());
    }
}
