/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2015, Caper Holdings, LLC
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <dtVoxel/voxelgrid.h>
#include <dtCore/project.h>
#include <dtUtil/log.h>
#include <dtUtil/matrixutil.h>
#include <dtUtil/fileutils.h>
#include <dtUtil/mathdefines.h>
#include <osgDB/WriteFile>
#include <osgUtil/Optimizer>
#include <osg/PagedLOD>
#include <iostream>
#include <fstream>
#include <tbb/parallel_for.h>

const char START_END_CHAR = '!';
const int FILE_IDENT = 99834;
const std::string DATABASE_FILENAME("VoxelGridDatabase.osgb");

namespace dtVoxel
{

   VoxelGrid::VoxelGrid()
      : mBlocksX(0)
      , mBlocksY(0)
      , mBlocksZ(0)
      , mNumBlocks(0)
      , mViewDistance(1000.0f)
      , mGridOffset()
      , mWSDimensions()
      , mBlockDimensions()
      , mCellDimensions()
      , mInitialized(false)
      , mRes0()
      , mRes1()
      , mRes2()
      , mRes3()
      , mDist0(100.0f)
      , mDist1(300.0f)
      , mDist2(550.0f)
      , mTextureResolution()
      , mRootNode(new osg::Group())
      , mBlocks(NULL)
   {

   }

   VoxelGrid::~VoxelGrid()
   {
   }

   void VoxelGrid::Init(const osg::Vec3& grid_offset, const osg::Vec3& dimensions, const osg::Vec3& block_dimensions, const osg::Vec3& cellDimensions, const osg::Vec3i& textureResolution)
   {
      mGridOffset = grid_offset;
      mWSDimensions = dimensions;
      mBlockDimensions = block_dimensions;
      mCellDimensions = cellDimensions;
      mTextureResolution = textureResolution;

      mBlocksX = int(std::floor(mWSDimensions[0] / mBlockDimensions[0]));
      mBlocksY = int(std::floor(mWSDimensions[1] / mBlockDimensions[1]));
      mBlocksZ = int(std::floor(mWSDimensions[2] / mBlockDimensions[2]));

      mNumBlocks = mBlocksX * mBlocksY * mBlocksZ;

      mBlocks = new VoxelBlock[mNumBlocks];


      mRes0 = mTextureResolution;

      mRes1.set(int(std::floor(float(mTextureResolution[0]) * 0.75f)),
         int(std::floor(float(mTextureResolution[1]) * 0.75f)),
         int(std::floor(float(mTextureResolution[2]) * 0.75f)));

      mRes2.set(int(std::floor(float(mRes1[0]) * 0.75f)),
         int(std::floor(float(mRes1[1]) * 0.75f)),
         int(std::floor(float(mRes1[2]) * 0.75f)));

      mRes3.set(1 + int(std::floor(float(mRes3[0]) * 0.75f)),
         1 + int(std::floor(float(mRes3[1]) * 0.75f)),
         1 + int(std::floor(float(mRes3[2]) * 0.75f)));



      mInitialized = true;
   }

   osg::BoundingBox VoxelGrid::ComputeWorldBounds(const osg::Vec3& pos)
   {
      osg::Vec3 bbMin;
      osg::Vec3 bbMax;

      osg::Vec3 clampMax = mGridOffset + mWSDimensions;
      
      
      for (int i = 0; i < 3; ++i)
      {
         bbMin[i] = pos[i] - mViewDistance;         
         dtUtil::Clamp(bbMin[i], mGridOffset[i], clampMax[i]);

         bbMax[i] = pos[i] + mViewDistance;
         dtUtil::Clamp(bbMax[i], mGridOffset[i], clampMax[i]);
      }

      return osg::BoundingBox(bbMin, bbMax);
   }

   void VoxelGrid::MarkDirtyAABB(const osg::BoundingBox& bb)
   {      
      osg::Vec3 newDim;
      newDim[0] = bb.xMax() - bb.xMin();
      newDim[1] = bb.yMax() - bb.yMin();
      newDim[2] = bb.zMax() - bb.zMin();

      int blocksX = int(std::floor(newDim.x() / mBlockDimensions.x()));
      int blocksY = int(std::floor(newDim.y() / mBlockDimensions.y()));
      int blocksZ = int(std::floor(newDim.z() / mBlockDimensions.z()));

      //std::cout << "Blocks X " << blocksX << " Blocks Y " << blocksY << " Blocks Z " << blocksZ << std::endl;

      osg::Vec3 bboffset = bb._min - mGridOffset;
      int startX = int(std::floor(bboffset.x() / mBlockDimensions.x()));
      int startY = int(std::floor(bboffset.y() / mBlockDimensions.y()));
      int startZ = int(std::floor(bboffset.z() / mBlockDimensions.z()));

      //std::cout << "start X " << startX << " start Y " << startY << " start Z " << startZ << std::endl;

      int endX = startX + blocksX;
      int endY = startY + blocksY;
      int endZ = startZ + blocksZ;

      //std::cout << "end X " << endX << " end Y " << endY << " end Z " << endZ << std::endl;

      for (int z = startZ; z < endZ; z++)
      {
         for (int y = startY; y < endY; y++)
         {
            for (int x = startX; x < endX; x++)
            {
               VoxelBlock* curBlock = GetBlockFromIndex(x, y, z);

               if (curBlock != nullptr)
               {
                  if (curBlock->IsAllocated())
                  {
                     //temporarily just recreating the whole block
                     //curBlock->RegenerateAABB(*mVoxelActor, bb, mTextureResolution);                     
                     
                     curBlock->DeAllocate();
                     curBlock->AllocateLODMesh(*mVoxelActor, mRes0, mDist0, mRes1, mDist1, mRes2, mDist2, mRes3, mViewDistance);

                  }
                  else
                  {
                     curBlock->SetDirty(true);
                  }
               }
            }
         }
      }
   }


   void VoxelGrid::UpdateGrid(const osg::Vec3& newCameraPos)
   {   
      return;

      osg::BoundingBox newBounds = ComputeWorldBounds(newCameraPos);
      
      osg::BoundingBox combinedBounds = mAllocatedBounds;

      combinedBounds.expandBy(newBounds);

      osg::Vec3 newDim;
      newDim[0] = combinedBounds.xMax() - combinedBounds.xMin();
      newDim[1] = combinedBounds.yMax() - combinedBounds.yMin();
      newDim[2] = combinedBounds.zMax() - combinedBounds.zMin();

      int blocksX = int(std::floor(newDim.x() / mBlockDimensions.x()));
      int blocksY = int(std::floor(newDim.y() / mBlockDimensions.y()));
      int blocksZ = int(std::floor(newDim.z() / mBlockDimensions.z()));

      //std::cout << "Blocks X " << blocksX << " Blocks Y " << blocksY << " Blocks Z " << blocksZ << std::endl;

      osg::Vec3 bboffset = combinedBounds._min - mGridOffset;
      int startX = int(std::floor(bboffset.x() / mBlockDimensions.x()));
      int startY = int(std::floor(bboffset.y() / mBlockDimensions.y()));
      int startZ = int(std::floor(bboffset.z() / mBlockDimensions.z()));

      int endX = startX + blocksX;
      int endY = startY + blocksY;
      int endZ = startZ + blocksZ;
    
      for (int z = startZ; z < endZ; z++)
      {
         for (int y = startY; y < endY; y++)
         {
            for (int x = startX; x < endX; x++)
            {
               VoxelBlock* curBlock = GetBlockFromIndex(x, y, z);

               if (curBlock != nullptr)
               {
                  osg::Vec3 centerPos = GetCenterOfBlock(x, y, z);

                  bool posInOldBounds = mAllocatedBounds.contains(centerPos);
                  bool posInNewBounds = newBounds.contains(centerPos);
                   
                  if (posInNewBounds) 
                  {
                     //allocate
                     if (!curBlock->GetEmpty() && !curBlock->IsAllocated())
                     {
                        int index = (z * mBlocksY * mBlocksX) + (y * mBlocksX) + x;

                        if (curBlock->GetDirty())
                        {
                           curBlock->AllocateLODMesh(*mVoxelActor, mRes0, mDist0, mRes1, mDist1, mRes2, mDist2, mRes3, mViewDistance);
                           mRootNode->addChild(curBlock->GetOSGNode());
                        }
                        else if (curBlock->LoadCachedModel(mCacheFolder, index))
                        {
                           mRootNode->addChild(curBlock->GetOSGNode());
                        }                        
                        else
                        {
                           LOG_ERROR("No data found for block " + dtUtil::ToString(index) );
                           //curBlock->AllocateCombinedMesh(*mVoxelActor, mTextureResolution);
                           //curBlock->SaveCachedModel(mCacheFolder, index);
                        }

                     }                     

                  }
                  else if (posInOldBounds)
                  {
                     if (curBlock->IsAllocated())
                     {
                        //deallocate
                        curBlock->DeAllocate();
                        mRootNode->removeChild(curBlock->GetOSGNode());
                     }
                  }

               }
               else
               {
                  LOG_ERROR("Invalid block dimensions calculated when updating grid.");
               }
            }
         }
      }

      mAllocatedBounds = newBounds;
   }


   void VoxelGrid::CreateGridFromActor(const osg::Vec3& pos, VoxelActor& voxelActor)
   {
      if (!mInitialized)
      {
         LOG_ERROR("VoxelGrid has not been initialized.");
         return;
      }

      mAllocatedBounds = ComputeWorldBounds(pos);
      
      mVoxelActor = &voxelActor;
      
      std::cout << std::endl << "Creating Voxel Grid with " << mNumBlocks << " Voxel Blocks" << std::endl;

      GenerateCacheString();

      
      //int blockCount = 0;
      OpenThreads::Atomic blockCount;
      for (int z = 0; z < mBlocksZ; z++)
      {
         
         tbb::parallel_for(tbb::blocked_range<int>(0,mBlocksY),
               [=, &blockCount](const tbb::blocked_range<int>& r)
               {
         for (int y=r.begin(); y!=r.end(); ++y)//(int y = 0; y < mBlocksY; y++)
         {
            for (int x = 0; x < mBlocksX; x++)
            {
               VoxelBlock* curBlock = GetBlockFromIndex(x, y, z);

               osg::Vec3 offsetFrom = mGridOffset;
               offsetFrom[0] += x * mBlockDimensions[0];
               offsetFrom[1] += y * mBlockDimensions[1];
               offsetFrom[2] += z * mBlockDimensions[2];

               osg::Vec3 offsetTo = offsetFrom + mBlockDimensions;

               curBlock->Init(mBlockDimensions, offsetFrom, mCellDimensions);
               
               osg::BoundingBox bb(offsetFrom, offsetTo);
               openvdb::GridBase::Ptr vdbGrid = mVoxelActor->CollideWithAABB(bb);

               //bool collides = mVoxelActor->FastSampleWithAABB(bb, osg::Vec3i(50, 50, 10));

               if (vdbGrid != NULL && !vdbGrid->empty())
               {
                  int index = (z * mBlocksY * mBlocksX) + (y * mBlocksX) + x;

                  //only allocate this block if it is within our visual bounds
                  if (mAllocatedBounds.contains(GetCenterOfBlock(x, y, z)))
                  {
                     //attempt to load from cache
                     if (!curBlock->LoadCachedModel(mCacheFolder, index))
                     {
                        //allocate this block
                        //curBlock->AllocateCombinedMesh(*mVoxelActor, mTextureResolution);
                        
                        curBlock->AllocateLODMesh(*mVoxelActor, mRes0, mDist0, mRes1, mDist1, mRes2, mDist2, mRes3, mViewDistance);

                        curBlock->SaveCachedModel(mCacheFolder, index);

                     }

                     mRootNode->addChild(curBlock->GetOSGNode());
                  }
                  else
                  {
                     if (!curBlock->HasCachedModel(mCacheFolder, index))
                     {
                        std::cout << "Caching model for later use" << std::endl;
                        //curBlock->AllocateCombinedMesh(*mVoxelActor, mTextureResolution);
                        curBlock->AllocateLODMesh(*mVoxelActor, mRes0, mDist0, mRes1, mDist1, mRes2, mDist2, mRes3, mViewDistance);

                        curBlock->SaveCachedModel(mCacheFolder, index);

                        curBlock->DeAllocate();
                     }
                  }
                  
                  curBlock->SetEmpty(false);
               }
               ++blockCount;
            }
            
            std::cout << std::endl << mNumBlocks - blockCount << " Blocks remaining." << std::endl;

         }
               });
      }

      std::cout << std::endl << "Done Creating Voxel Grid" << std::endl;

      /*std::cout << std::endl << "Optimizing Grid" << std::endl;
      osgUtil::Optimizer opt;
      opt.optimize(mRootNode, osgUtil::Optimizer::SPATIALIZE_GROUPS);
      std::cout << std::endl << "Done Optimizing" << std::endl;*/
   }

   void VoxelGrid::CreatePagedLODGrid(const osg::Vec3& pos, VoxelActor& voxelActor)
   {
      if (!mInitialized)
      {
         LOG_ERROR("VoxelGrid has not been initialized.");
         return;
      }

      mAllocatedBounds = ComputeWorldBounds(pos);

      mVoxelActor = &voxelActor;

      std::cout << std::endl << "Creating Voxel Grid with " << mNumBlocks << " Voxel Blocks" << std::endl;

      GenerateCacheString();

      if (!ReadBlockVisibility())
      {
         GenerateVisibility(voxelActor);

         bool success = WriteBlockVisibility();

         if (!success)
         {
            LOG_ERROR("Error writing visibility information.");
         }
      }

      
      OpenThreads::Atomic blockCount;
      for (int z = 0; z < mBlocksZ; z++)
      {         
         tbb::parallel_for(tbb::blocked_range<int>(0, mBlocksY),
            [=, &blockCount](const tbb::blocked_range<int>& r)
         {
            for (int y = r.begin(); y != r.end(); ++y)//(int y = 0; y < mBlocksY; y++)
            {
               for (int x = 0; x < mBlocksX; x++)
               {
                  VoxelBlock* curBlock = GetBlockFromIndex(x, y, z);

                  osg::Vec3 offsetFrom = mGridOffset;
                  offsetFrom[0] += x * mBlockDimensions[0];
                  offsetFrom[1] += y * mBlockDimensions[1];
                  offsetFrom[2] += z * mBlockDimensions[2];

                  osg::Vec3 offsetTo = offsetFrom + mBlockDimensions;

                  curBlock->Init(mBlockDimensions, offsetFrom, mCellDimensions);

                  osg::BoundingBox bb(offsetFrom, offsetTo);

                  int index = (z * mBlocksY * mBlocksX) + (y * mBlocksX) + x;

                  if (!mBlockVisibility[index])
                  {
                     if (!curBlock->LoadCachedModel(mCacheFolder, index))
                     {
                        curBlock->WritePagedLOD(*mVoxelActor, index, mCacheFolder, mRes0, mDist0, mRes1, mDist1, mRes2, mDist2, mRes3, mViewDistance);
                     }

                     mRootNode->addChild(curBlock->GetOSGNode());

                     curBlock->SetEmpty(false);
                  }
                  ++blockCount;
               }

               std::cout << std::endl << mNumBlocks - blockCount << " Blocks remaining." << std::endl;

            }
         });
      }

      std::cout << std::endl << "Done Creating Voxel Grid" << std::endl;

      /*std::cout << std::endl << "Optimizing Grid" << std::endl;
      osgUtil::Optimizer opt;
      opt.optimize(mRootNode, osgUtil::Optimizer::SPATIALIZE_GROUPS);
      std::cout << std::endl << "Done Optimizing" << std::endl;*/
   }


   void VoxelGrid::GenerateVisibility(VoxelActor& voxelActor)
   {
      if (!mInitialized)
      {
         LOG_ERROR("VoxelGrid has not been initialized.");
         return;
      }
    
      std::cout << std::endl << "Generating Visibility" << std::endl;
      
      mBlockVisibility.resize(mNumBlocks);
      
      //int blockCount = 0;
      OpenThreads::Atomic blockCount;
      for (int z = 0; z < mBlocksZ; z++)
      {
         
         tbb::parallel_for(tbb::blocked_range<int>(0, mBlocksY),
            [=, &blockCount](const tbb::blocked_range<int>& r)
         {
            for (int y = r.begin(); y != r.end(); ++y)//(int y = 0; y < mBlocksY; y++)
            {
               for (int x = 0; x < mBlocksX; x++)
               {
                  osg::Vec3 offsetFrom = mGridOffset;
                  offsetFrom[0] += x * mBlockDimensions[0];
                  offsetFrom[1] += y * mBlockDimensions[1];
                  offsetFrom[2] += z * mBlockDimensions[2];

                  osg::Vec3 offsetTo = offsetFrom + mBlockDimensions;

                  osg::BoundingBox bb(offsetFrom, offsetTo);
                  openvdb::GridBase::Ptr vdbGrid = mVoxelActor->CollideWithAABB(bb);

                  int index = (z * mBlocksY * mBlocksX) + (y * mBlocksX) + x;

                  if (vdbGrid != NULL && !vdbGrid->empty())
                  {                     
                     mBlockVisibility[index] = false;
                  }
                  else
                  {
                     mBlockVisibility[index] = true;
                  }
                  
                  ++blockCount;
               }

               std::cout << std::endl << mNumBlocks - blockCount << " Blocks remaining." << std::endl;
            }
         });
      }

      std::cout << std::endl << "Done Generating Visibility" << std::endl;

      /*std::cout << "Visibility Table" << std::endl;

      for(int i = 0; i < mNumBlocks; ++i)
      {
         std::cout << dtUtil::ToString(mBlockVisibility[i]) << "   ";
      }

      std::cout << std::endl << std::endl;*/
   }



   VoxelBlock* VoxelGrid::GetBlockFromIndex(int x, int y, int z)
   {
      VoxelBlock* block = NULL;

      int index = (z * mBlocksY * mBlocksX) + (y * mBlocksX) + x;
      
      if (index >= 0 && index < mNumBlocks)
      {
         block = &mBlocks[index];
      }
      else
      {
         LOG_ERROR("Block index out of bounds.");
      }
      
      return block;
   }

   osg::Vec3 VoxelGrid::GetCenterOfBlock(int x, int y, int z)
   {
      float blockX = x * mBlockDimensions.x();
      float blockY = y * mBlockDimensions.y();
      float blockZ = z * mBlockDimensions.z();

      osg::Vec3 pos(blockX, blockY, blockZ);

      osg::Vec3 halfBlock = mBlockDimensions * 0.5;

      pos += halfBlock;
      pos += mGridOffset;
      
      return pos;
   }


   VoxelBlock* VoxelGrid::GetBlockFromIndex(int index)
   {
      VoxelBlock* block = NULL;

      if (index >= 0 && index < mNumBlocks)
      {
         block = &mBlocks[index];
      }
      else
      {
         LOG_ERROR("Block index out of bounds.");
      }

      return block;
   }

   VoxelBlock* VoxelGrid::GetBlockFromPos(const osg::Vec3& pos)
   {
      VoxelBlock* block = NULL;
      osg::Vec3 offset = pos - mGridOffset;
      int cellX = int(std::floor(pos[0] / mBlockDimensions[0]));
      int cellY = int(std::floor(pos[1] / mBlockDimensions[1]));
      int cellZ = int(std::floor(pos[2] / mBlockDimensions[2]));

      int index = (cellZ * mBlocksY * mBlocksX) + (cellY * mBlocksX) + cellX;

      if (index > 0 && index < mNumBlocks)
      {
         block = &mBlocks[index];
      }

      return block;
   }

   const osg::Vec3i& VoxelGrid::GetTextureResolution() const
   {
      return mTextureResolution;
   }

   osg::Node* VoxelGrid::GetOSGNode()
   {
      return mRootNode.get();
   }

   const osg::Node* VoxelGrid::GetOSGNode() const
   {
      return mRootNode.get();
   }
   
   bool VoxelGrid::ReadBlockVisibility()
   {
      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + mCacheFolder + "/";
      std::string filename("VisibilityCache.dat");
      std::ifstream inFile(filePath + filename, std::ios::in | std::ios::binary);

      if (!inFile.fail())
      {
         //get length of file
         inFile.seekg(0, std::ios::end);
         int length = inFile.tellg();
         inFile.seekg(0, std::ios::beg);

         //read data
         char* buffer = new char[length];
         inFile.read(buffer, length);

         dtUtil::DataStream ds(buffer, length);

         char startChar = 'a';
         ds.Read(startChar);

         if (startChar != START_END_CHAR)
         {
            return false;
         }

         //write magic number
         int magicNum = 0;
         ds.Read(magicNum);

         if (magicNum != FILE_IDENT)
         {
            return false;
         }

         int numBlocks = 0;
         ds.Read(numBlocks);

         if (numBlocks != mNumBlocks)
         {
            LOG_ERROR("Unexpected number of voxel blocks in visibility file.");
         }

         mBlockVisibility.resize(mNumBlocks);

         for (int i = 0; i < mNumBlocks; ++i)
         {
            bool b = false;
            ds.Read(b);

            mBlockVisibility[i] = b;
         }
         
         inFile.close();
         
         return !inFile.fail();
      }
      else
      {
         LOG_ERROR("Error reading visibility data.");
         return false;
      }
   }

   bool VoxelGrid::WriteBlockVisibility()
   {      
      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + mCacheFolder + "/";
      std::string filename("VisibilityCache.dat");
      std::ofstream outFile(filePath + filename, std::ios::out | std::ios::binary);
      
      if (!outFile.fail())
      {
         dtUtil::DataStream ds;
         ds.Write(START_END_CHAR);
         ds.Write(FILE_IDENT);

         ds.Write(mNumBlocks);

         for (int i = 0; i < mNumBlocks; ++i)
         {
            ds.Write(mBlockVisibility[i]);
         }
         
         outFile.write(ds.GetBuffer(), ds.GetBufferSize());

         outFile.close();
         
         return !outFile.fail();

      }
      else
      {
         LOG_ERROR("Error writing visibility data.");
         return false;
      }

   }

   void VoxelGrid::GenerateCacheString()
   {
      std::stringstream folderName;

      folderName << "Volumes/cache/" << mVoxelActor->GetDatabase().GetResourceName() << "_" << mBlocksX << mBlocksY << mBlocksZ;

      mCacheFolder = folderName.str();

      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + mCacheFolder + "/";

      dtUtil::FileUtils::GetInstance().MakeDirectory(filePath);

   }

   bool VoxelGrid::WriteVoxelDatabase()
   {
      bool result = false;

      std::string filePath = mCacheFolder;
      
      std::stringstream fileName;

      fileName << filePath << DATABASE_FILENAME;

      if (dtUtil::FileUtils::GetInstance().FileExists(fileName.str()))
      {
         result = osgDB::writeNodeFile(*mRootNode, fileName.str());

         std::cout << "Writing master voxel database " << fileName.str() << std::endl;
      }

      return result;
   }

   bool VoxelGrid::ReadVoxelDatabase()
   {
      bool result = false;

      std::string filePath = mCacheFolder;

      std::stringstream fileName;

      fileName << filePath << DATABASE_FILENAME;

      std::cout << "Reading master voxel database." << fileName.str() << std::endl;

      if (dtUtil::FileUtils::GetInstance().FileExists(fileName.str()))
      {
         osg::Node* n = dtUtil::FileUtils::GetInstance().ReadNode(fileName.str());
         if (n != nullptr)
         {
            mRootNode->addChild(n);
            result = true;
         }
         else
         {
            LOG_ERROR("Error reading voxel database, read result = NULL.");
         }

      }
      else
      {
         LOG_ERROR("Error reading voxel database, file does not exist.");
      }

      return result;
   }

} /* namespace dtVoxel */