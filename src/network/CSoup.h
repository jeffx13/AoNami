#ifndef HTMLPARSER_H
#define HTMLPARSER_H
#include <QString>
#include <iostream>

#include <tidy.h>
#include <tidybuffio.h>
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include <pugixml/pugixml.hpp>



class CSoup
{
private:
    pugi::xml_document doc;
public:
    CSoup(std::string html) {
        parse(tidy(html.data()).data());
    }

    ~CSoup(){}

    inline std::string selectText(std::string XPath){
        return doc.select_node(XPath.data()).node().child_value ();
    }
    inline pugi::xpath_node_set select(std::string XPath){
        return doc.select_nodes(XPath.data());
    }
    inline pugi::xpath_node selectFirst(const char* XPath){
        return doc.select_node(XPath);
    }

private:
    std::string tidy(const char* input){
        TidyBuffer output = {0};
        TidyBuffer errbuf = {0};
        int rc = -1;
        Bool ok;

        TidyDoc tdoc = tidyCreate();                     // Initialize "document"

        ok = tidyOptSetBool( tdoc, TidyXhtmlOut, yes );  // Convert to XHTML
        if ( ok )
            rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
        if ( rc >= 0 )
            rc = tidyParseString( tdoc, input);           // Parse the input
        if ( rc >= 0 )
            rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
        if ( rc >= 0 )
            rc = tidyRunDiagnostics( tdoc );               // Kvetch
        if ( rc > 1 )                                    // If error, force output.
            rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
        if ( rc >= 0 )
            rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

        if ( rc >= 0 )
        {
//            if ( rc > 0 )
//            printf( "\nDiagnostics:\n\n%s", errbuf.bp );
//            printf( "\nAnd here is the result:\n\n%s", output.bp );
        }
        else
            printf( "A severe error (%d) occurred.\n", rc );

        tidyBufFree( &errbuf );
        tidyRelease( tdoc );
        int len = strlen(reinterpret_cast<const char*>(output.bp));
        std::string str2(output.bp, output.bp + len);
        tidyBufFree( &output );
        return str2;
    }

    bool parse(const char* source){
        pugi::xml_parse_result parsed = doc.load_string(source);
        if (parsed)
        {
//            std::cout << "XML [" << source << "] parsed without errors, attr value: [" << doc.child("html").attribute("xmlns").value() << "]\n\n";
            return true;
        }
        else
        {
            std::cout << "XML [" << source << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
            std::cout << "Error description: " << parsed.description() << "\n";
            std::cout << "Error offset: " << parsed.offset << " (error at [..." << (source + parsed.offset) << "]\n\n";
        }
        return false;
    }
};

#endif // HTMLPARSER_H
