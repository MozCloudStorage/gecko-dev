
/*
 The contents of this file are subject to the Mozilla Public
 License Version 1.1 (the "License"); you may not use this file
 except in compliance with the License. You may obtain a copy of
 the License at http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS
 IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 implied. See the License for the specific language governing
 rights and limitations under the License.

 The Original Code is mozilla.org code.

 The Initial Developer of the Original Code is Sun Microsystems,
 Inc. Portions created by Sun are
 Copyright (C) 1999 Sun Microsystems, Inc. All
 Rights Reserved.

 Contributor(s):
*/

package org.mozilla.dom.test;

import java.util.*;
import java.io.*;
import org.mozilla.dom.test.*;
import org.mozilla.dom.*;
import org.w3c.dom.*;

public class DocumentImpl_importNode_Node_boolean_3 extends BWBaseTest implements Execution
{

   /**
    *
    ***********************************************************
    *  Constructor
    ***********************************************************
    *
    */
   public DocumentImpl_importNode_Node_boolean_3()
   {
   }


   /**
    *
    ***********************************************************
    *  Starting point of application
    *
    *  @param   args    Array of command line arguments
    *
    ***********************************************************
    */
   public static void main(String[] args)
   {
   }

   /**
    ***********************************************************
    *
    *  Execute Method 
    *
    *  @param   tobj    Object reference (Node/Document/...)
    *  @return          true or false  depending on whether test passed or failed.
    *
    ***********************************************************
    */
   public boolean execute(Object tobj)
   {
      if (tobj == null)  {
           TestLoader.logPrint("Object is NULL...");
           return BWBaseTest.FAILED;
      }

      String os = System.getProperty("OS");
      osRoutine(os);
      setUnsupported();


      DocumentImpl d = (DocumentImpl)tobj;
      if (d != null)
      {
        Element e = null;
        try {
             e = d.createElement("HR");
	     if (e == null) {
                TestLoader.logErrPrint("Document 'createElement(HR) is  NULL..");
                return BWBaseTest.FAILED;
             }
         } catch (Exception ex) {
             String msg = "createElement Exception:" + ex ; 
             TestLoader.logErrPrint(msg);
             return BWBaseTest.FAILED;
         }

         try {
             Node n= d.importNode(e, true);
             TestLoader.logErrPrint("Document 'importNode(null,false) is  not supported method...");
             return BWBaseTest.FAILED;
        } catch (UnsupportedOperationException ue) {
             String msg = "UNSUPPORTED METHOD  " ; 
             TestLoader.logErrPrint(msg);
             return BWBaseTest.PASSED;
        } catch (DOMException de) {
             String msg = "Caught DOMException " + de; 
             TestLoader.logErrPrint(msg);
             return BWBaseTest.PASSED;
 
        }
      } else {
             System.out.println("Document is  NULL..");
             return BWBaseTest.FAILED;
      }
   }

   /**
    *
    ***********************************************************
    *  Routine where OS specific checks are made. 
    *
    *  @param   os      OS Name (SunOS/Linus/MacOS/...)
    ***********************************************************
    *
    */
   private void osRoutine(String os)
   {
     if(os == null) return;

     os = os.trim();
     if(os.compareTo("SunOS") == 0) {}
     else if(os.compareTo("Linux") == 0) {}
     else if(os.compareTo("Windows") == 0) {}
     else if(os.compareTo("MacOS") == 0) {}
     else {}
   }
}
