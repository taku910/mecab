import org.chasen.mecab.MeCab;
import org.chasen.mecab.Tagger;
import org.chasen.mecab.Node;

public class test {
  static {
    try {
       System.loadLibrary("MeCab");
    } catch (UnsatisfiedLinkError e) {
       System.err.println("Cannot load the example native code.\nMake sure your LD_LIBRARY_PATH contains \'.\'\n" + e);
       System.exit(1);
    }
  }

  public static void main(String[] argv) {
     System.out.println(MeCab.VERSION);
     Tagger tagger = new Tagger();
     String str = "太郎は二郎にこの本を渡した。";
     System.out.println(tagger.parse(str));
     Node node = tagger.parseToNode(str);
     for (;node != null; node = node.getNext()) {
	System.out.println(node.getSurface() + "\t" + node.getFeature());
     }
     System.out.println ("EOS\n");
  }
}
