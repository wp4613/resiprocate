#ifndef PlainContents_hxx
#define PlainContents_hxx

#include "sip2/sipstack/Contents.hxx"
#include "sip2/util/Data.hxx"

namespace Vocal2
{

class PlainContents : public Contents
{
   public:
      PlainContents();
      PlainContents(const Data& text);
      PlainContents(HeaderFieldValue* hfv, const Mime& contentType);
      PlainContents(const PlainContents& rhs);
      virtual ~PlainContents();
      PlainContents& operator=(const PlainContents& rhs);

      virtual Contents* clone() const;

      virtual const Mime& getType() const;

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual void parse(ParseBuffer& pb);

      Data& text() {checkParsed(); return mText;}
      
   private:
      static ContentsFactory<PlainContents> Factory;

      Data mText;
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 */