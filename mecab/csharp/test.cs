using System;

public class runme {
  static void Main(String[] argv) {
    try {
	Console.WriteLine (MeCab.MeCab.VERSION);
    
      String arg = "";
      for (int i = 0; i < argv.Length; ++i) {
	arg += " ";
	arg += argv[i];
      }
      MeCab.Tagger t = new MeCab.Tagger (arg);
      String s = "太郎は花子が持っている本を次郎に渡した。";
      Console.WriteLine (t.parse(s));
	    
      for (MeCab.Node n = t.parseToNode(s); n != null ; n = n.next) {
	Console.WriteLine (n.surface + "\t" + n.feature);
      }
     }

     catch (Exception e) {
	Console.WriteLine("Generic Exception Handler: {0}", e.ToString());
     }
  }
}
