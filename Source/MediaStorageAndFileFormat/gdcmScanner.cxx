/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmScanner.h"
#include "gdcmReader.h"
#include "gdcmGlobal.h"
#include "gdcmDicts.h"
#include "gdcmDict.h"
#include "gdcmDictEntry.h"

namespace gdcm
{


Scanner::~Scanner()
{
}

void Scanner::AddTag( Tag const & t )
{
  static const Global &g = GlobalInstance;
  static const Dicts &dicts = g.GetDicts();
  const DictEntry &entry = dicts.GetDictEntry( t );
  // Is this tag an ASCII on ?
  if( entry.GetVR() & VR::VRASCII )
    {
    Tags.insert( t );
    }
  else if( entry.GetVR() == VR::INVALID )
    {
    gdcmWarningMacro( "Only tag with known VR are allowed. Tag " << t << " will be discarded" );
    }
  else
    {
    assert( entry.GetVR() & VR::VRBINARY );
    gdcmWarningMacro( "Only ASCII VR are supported for now. Tag " << t << " will be discarded" );
    }
}

bool Scanner::Scan( Directory::FilenamesType const & filenames )
{
  // Is there at least one tag ?
  if( Tags.empty() ) return true;

  // Find the tag with the highest value (get the one from the end of the std::set)
  TagsType::const_reverse_iterator it1 = Tags.rbegin();
  const Tag & last = *it1;

  Directory::FilenamesType::const_iterator it = filenames.begin();
  for(; it != filenames.end(); ++it)
    {
    Reader reader;
    const char *filename = it->c_str();
    assert( filename );
    reader.SetFileName( filename );
    bool read = false;
    try
      {
      // Start reading all tags, including the 'last' one:
      read = reader.ReadUpToTag(last);
      }
    catch(std::exception & ex)
      {
      gdcmWarningMacro( "Failed to read:" << filename << " with ex:" << ex.what() );
      }
    catch(...)
      {
      gdcmWarningMacro( "Failed to read:" << filename  << " with unknown error" );
      }
    if( read )
      {
      const DataSet & ds = reader.GetFile().GetDataSet();
      TagsType::const_iterator tag = Tags.begin();
      for( ; tag != Tags.end(); ++tag )
        {
        if( ds.FindDataElement( *tag ) )
          {
          DataElement const & de = ds.GetDataElement( *tag );
          const ByteValue *bv = de.GetByteValue();
          //assert( VR::IsASCII( vr ) );
          if( bv ) // Hum, should I store an empty string or what ?
            {
            std::string s( bv->GetPointer(), bv->GetLength() );
            s.resize( std::min( s.size(), strlen( s.c_str() ) ) );
            // Store the potentially new value:
            Values.insert( s );
            assert( Values.find( s ) != Values.end() );
            const char *value = Values.find( s )->c_str();
            assert( value );
            // Keep the mapping:
            FilenameToValue &mapping = Mappings[*tag];
            mapping.insert(
              FilenameToValue::value_type(filename, value));
            }
          }
        }
      }
    }
  return true;
}

void Scanner::Print( std::ostream & os ) const
{
  os << "Values:\n";
  ValuesType::const_iterator it = Values.begin();
  for( ; it != Values.end(); ++it)
    {
    os << *it << "\n";
    }
  os << "Mapping:\n";
  TagsType::const_iterator tag = Tags.begin();
  for( ; tag != Tags.end(); ++tag )
    {
    os << "Tag: " << *tag << "\n";
    //const FilenameToValue &mapping = Mappings[*tag];
    if( Mappings.find(*tag) != Mappings.end() )
      {
      const FilenameToValue &mapping = GetMapping(*tag);
      FilenameToValue::const_iterator it = mapping.begin();
      for( ; it != mapping.end(); ++it)
        {
        const char *filename = it->first;
        const char *value = it->second;
        os << filename << " -> " << value << "\n";
        }
      }
    }
}

Scanner::FilenameToValue const & Scanner::GetMapping(Tag const &t) const
{
  assert( Mappings.find(t) != Mappings.end() );
  return Mappings.find(t)->second;
}

const char* Scanner::GetValue(Tag const &t, const char *filename) const
{
  FilenameToValue const &ftv = GetMapping(t);
  if( ftv.find(filename) != ftv.end() )
    {
    return ftv.find(filename)->second;
    }
  return NULL;
}

} // end namespace gdcm
