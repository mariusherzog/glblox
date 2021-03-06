////////////////////////////////////////////////////////////////////////////////
// Small Volume   A utility class for storing and accessing chunk data.     
//                                                                        
// @author         Peter A Bennett
// @copyright      (c) 2012 Peter A Bennett
// @license        LGPL      
// @email          pab850@googlemail.com
// @contact        www.bytebash.com
//
////////////////////////////////////////////////////////////////////////////////

#include "smallVolume.hpp"

bool operator==(const vector4f &a, const vector4f &b)
{
   return a.x == b.x &&
          a.y == b.y &&
          a.z == b.z &&
          a.w == b.w;
}

bool operator==(const Position &a, const Position &b)
{
  return a.tuple.get<0>() == b.tuple.get<0>() &&
         a.tuple.get<1>() == b.tuple.get<1>() &&
         a.tuple.get<2>() == b.tuple.get<2>();
}

bool operator<(const Position &a, const Position &b)
{
   if (a.tuple.get<0>() < b.tuple.get<0>())
     return true;
   if (b.tuple.get<0>() < a.tuple.get<0>())
     return false;
   // a1==b1: continue with element 2
   if (a.tuple.get<1>() < b.tuple.get<1>())
     return true;
   if (b.tuple.get<1>() < a.tuple.get<1>())
     return false;
   // a2 == b2: continue with element 3
   if (a.tuple.get<2>() < b.tuple.get<2>())
     return true;
   return false; // early out
}

std::size_t hash_value(const Position &e)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, e.tuple.get<0>());
  boost::hash_combine(seed, e.tuple.get<1>());
  boost::hash_combine(seed, e.tuple.get<2>());
  return seed;
}
  
smallVolume::smallVolume(int sz)
{
   compressedFull = false;
   compressedArb = false;
   size = sz;
   modified = false;
}

void smallVolume::fill()
{
   compressedFull = true;
   volumeData.clear();
}

bool smallVolume::blockLeftVisible(int x, int y, int z)
{
   return not(is_solid(x-1,y,z));
}

bool smallVolume::blockRightVisible(int x, int y, int z)
{
   return not(is_solid(x+1,y,z));
}

bool smallVolume::blockAboveVisible(int x, int y, int z)
{
   return not(is_solid(x,y+1,z));
}

bool smallVolume::blockBelowVisible(int x, int y, int z)
{
   return not(is_solid(x,y-1,z));
}

bool smallVolume::blockFrontVisible(int x, int y, int z)
{
   return not(is_solid(x,y,z+1));
}

bool smallVolume::blockBackVisible(int x, int y, int z)
{
   return not(is_solid(x,y,z-1));
}

bool smallVolume::topBorderFull()
{
   if(!compressedFull)
   {
      int y = size-1;
      for(int x = 0; x < size; x++)
      {
         for(int z = 0; z < size; z++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool smallVolume::bottomBorderFull()
{
   if(!compressedFull)
   {
      int y = 0;
      for(int x = 0; x < size; x++)
      {
         for(int z = 0; z < size; z++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool smallVolume::rightBorderFull()
{
   if(!compressedFull)
   {
      int x = size-1;
      for(int y = 0; y < size; y++)
      {
         for(int z = 0; z < size; z++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool smallVolume::leftBorderFull()
{
   if(!compressedFull)
   {
      int x = 0;
      for(int y = 0; y < size; y++)
      {
         for(int z = 0; z < size; z++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}

bool smallVolume::frontBorderFull()
{
   if(!compressedFull)
   {
      int z = size-1;
      for(int x = 0; x < size; x++)
      {
         for(int y = 0; y < size; y++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}


bool smallVolume::backBorderFull()
{
   if(!compressedFull)
   {
      int z = 0;
      for(int x = 0; x < size; x++)
      {
         for(int y = 0; y < size; y++)
         {
            if(!is_solid(x,y,z))
            {
               return false;
            }
         }
      }
   }
   return true;
}

void smallVolume::empty()
{
   compressedFull = false;
   volumeData.clear();
}

bool smallVolume::is_solid(int x, int y, int z) 
{
   if(compressedFull)
   {
      return true;
   }
   Position key(x,y,z);
   return not (volumeData.find(key) == volumeData.end());
}

byte smallVolume::get(int x, int y, int z)
{
   Position key(x,y,z);
   if(is_solid(x,y,z))
   {
      return volumeData[key].blockType;
   }
   else
   {
      return 0;
   }
}

// Set the specified voxel to the specified value.
void smallVolume::set(int x, int y, int z, byte value)
{
   x = x>=size?size-1:x; // clamp
   x = x<0?0:x;
   y = y>=size?size-1:y;
   y = y<0?0:y;
   z = z>=size?size-1:z;
   z = z<0?0:z;
   
   Position key(x,y,z);
   block element;
   if(value > 0)
   {
      if(volumeData[key].blockType != value)
      {
         element.blockType = value;
         volumeData[key] = element;
         modified = true;
      }
   }
   else if(is_solid(x,y,z))
   {
      volumeData.erase(key);
      modified = true;
   }
}

void smallVolume::yRangeSet(int x, int y0, int y1, int z, byte value)
{
   // If an invalid range is given don't proceed.
   // Start index must be less than or equal to finish index.
   // Start index and finish index must be greater or equal to 0
   // Start index and finish index must be less than size
   if(y0 > y1 or y0 < 0 or y1 < 0 or y0 >= size or y1 >= size)
   {
      std::cout << "CPP: A valid range must be specified (smallVolume::yRangeSet)" << std::endl;
      return;
   }
   if(value == 0)
   {
      // Delete all values in the given range...
      for(int y=y0; y<y1; y++)
      {
         Position key(x,y,z);
         volumeData.erase(key);
      }
   }
   else
   {
      // Set all values in the given range...
      for(int y=y0; y<=y1; y++)
      {
         Position key(x,y,z);
         volumeData[key].blockType = value;
      }
   }
   modified = true;
}

bool smallVolume::is_empty()
{
   return volumeData.empty() and not compressedFull;
}

bool smallVolume::is_modified()
{
   return modified;
}


bool smallVolume::is_compressed()
{
   return compressedFull or compressedArb;
}

void smallVolume::uncompress()
{
   if(compressedFull)
   {
      for(int x = 0; x < size; x++)
      {
         for (int y = 0; y < size; y++)
         {
            for (int z = 0; z < size; z++)
            {
               set(x,y,z,1);
            }
         }
      }
      compressedFull = false;
   }
}

// Clear the modified flag.
void smallVolume::clearModifiedState()
{
   modified = false;
}

bool smallVolume::is_full()
{
   return compressedFull or volumeData.size() == (size*size*size);
}