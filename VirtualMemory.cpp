#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/**
 * This function fills the given frame with zeros
 * @param frameIndex - the frame to fill
 */
void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/**
 * Initialize the virtual memory
 */
void VMinitialize()
{
    clearTable(0);
}

/**
 * Calculate the cyclic distance from p
 * @param p1 -p
 * @param p2 - page to check
 * @return cyclic distance from p
 */
uint64_t calc_cyclic(uint64_t p1, uint64_t p2)
{
    uint64_t dis;
    if (p2 < p1)
    {
        dis = p1 - p2;
    }
    else
    {
        dis = p2 - p1;
    }

    if (dis >= NUM_PAGES - dis)
    {
        return NUM_PAGES - dis;
    }
    else
    {
        return dis;
    }
}

/**
 * Calculate the offset from virtualAddress
 * @return the offset
 */
uint64_t calcOffset(uint64_t virtualAddress)
{
    return virtualAddress - ((virtualAddress >> OFFSET_WIDTH) << OFFSET_WIDTH);
}


/**
 * Update the parameters of the frame that should be evicted according to the cyclic distance.
 * @param p - physical address
 * @param curPage - the page of the received frame
 * @param oldCycDist - the old cyclic distance to compare
 * @param frameToEvict - save here the frame to evict
 * @param pageToEvict - save here the page to evict
 * @param addressToEvict - save here the address to evict
 * @param curFrame - check of this frame needs to be evicted
 * @param prevFrame - the frame pointing to curFrame
 * @param offset - the offset of the curPage
 * @param prevFrameToEvict - save here the frame pointing to the frame to evict
 * @return true if the parameters was updated
 */
bool saveFrameToEvict(uint64_t p, uint64_t curPage, uint64_t &oldCycDist, word_t &frameToEvict,
                      word_t &pageToEvict, uint64_t &addressToEvict, word_t curFrame,
                      word_t prevFrame, int offset)
{
    uint64_t newCycDist = calc_cyclic(p, curPage);
    if (newCycDist > oldCycDist)
    {
        oldCycDist = newCycDist;

        // save the data of the frame to evict
        frameToEvict = curFrame;
        pageToEvict = (word_t) curPage;
        addressToEvict = ((word_t) prevFrame) * PAGE_SIZE + offset;
        return true;
    }
    return false;
}


/**
 * Update the parameters of the empty frame found while searching the memory
 * @param emptyFrame - save here the frame to empty
 * @param emptyAdd - save here the address to empty
 * @param frame - the empty frame
 * @param prevFrame - the frame pointing to the empty frame
 * @param offset - the offset of the empty page
 */
void saveEmptyFrame(word_t &emptyFrame, word_t &emptyAdd,
        word_t frame, word_t prevFrame, uint64_t offset)
{
    emptyFrame = frame;
    emptyAdd = ((word_t) prevFrame) * PAGE_SIZE + offset;
}


/**
 * This function operates the DFS- searching recursively throw the physical memory
 * @param p - physical address
 * @param frameToEvict - save here the frame to evict
 * @param pageToEvict - save here the page to evict
 * @param addressToEvict - save here the address to evict
 * @param emptyFrame - save here the frame to empty
 * @param emptyAdd - save here the address to empty
 * @param prevFrameToEvict - the frame pointing to the frame to evict
 * @param prevFrameToEmpty - the frame pointing to the empty frame
 * @param curFrame - the frame we are looking at
 * @param oldCycDist- the maximal cyclic distance that has been seen so far
 * @param maxFrame - the biggest frame index that has been seen so far
 * @param used - a boolean array of the used frames
 * @param prevFrame - the frame pointing at the curFrame
 * @param curPage - the page of the curFrame
 * @param curDepth - the depth in the imaginary memory tree
 * @param offset - the oofset of the curFrame
 */
void DFS(uint64_t p, word_t &frameToEvict, word_t &pageToEvict, uint64_t &addressToEvict, word_t
        &emptyFrame, word_t &emptyAdd, word_t &curFrame, uint64_t &oldCycDist, uint64_t &maxFrame,
        bool *used, word_t prevFrame = 0, uint64_t curPage = 0, uint64_t curDepth = 0, int offset
        = 0)
{
    if ((uint64_t) curFrame > maxFrame)
    {
        maxFrame = curFrame;
    }

    // in case we reached a "leaf"-page
    if (curDepth == TABLES_DEPTH)
    {
        if (!used[curFrame])
        {
            saveFrameToEvict(p, curPage, oldCycDist, frameToEvict, pageToEvict, addressToEvict,
                             curFrame, prevFrame, offset);
        }
        return;
    }

    word_t nextFrame = 0;
    bool isEmpty = true;
    for (uint64_t page = 0; page < PAGE_SIZE; page++)
    {
        PMread(((uint64_t)(curFrame)) * PAGE_SIZE + page, &nextFrame);
        if (nextFrame)
        {
            uint64_t nextPage = (curPage << OFFSET_WIDTH) + page;
            DFS(p, frameToEvict, pageToEvict, addressToEvict, emptyFrame, emptyAdd,
                nextFrame, oldCycDist, maxFrame, used, curFrame, nextPage,
                curDepth + 1, page);
//            if(emptyFrame){
//                return;
//            }
            isEmpty = false;
        }
    }
    if (isEmpty && !used[curFrame])
    {
        saveEmptyFrame(emptyFrame, emptyAdd, curFrame, prevFrame, offset);
    }
}

/**
 * Initialize array whit all offsets
 * @param pageArray - array whit all offsets
 * @param virtualAddress - the received virtual address from which we calculate the offsets
 */
void initializePageArray(uint64_t pageArray[], uint64_t virtualAddress)
{
    for (int depth = TABLES_DEPTH; depth >= 0; depth--)
    {
        if (depth != 0)
        {
            pageArray[depth] = virtualAddress & ((uint64_t)(PAGE_SIZE - 1));
            virtualAddress = virtualAddress >> OFFSET_WIDTH;
        }
        else
        {
            pageArray[0] = virtualAddress;
        }
    }
}


/**
 * This function calculate the physical address from the given virtual address using a recursive
 * search in the physical memory
 * @param virtualAddress - the received virtual address
 * @return -the physical address
 */
uint64_t getPhysicalAddress(uint64_t virtualAddress)
{
    uint64_t physicalAddress;
    word_t prevFrame = 0, curFrame = 0;
    uint64_t maxFrame = 0;
    uint64_t pageArray[TABLES_DEPTH];
    bool used[NUM_FRAMES] = {true};

    uint64_t p = virtualAddress >> OFFSET_WIDTH;
    initializePageArray(pageArray, virtualAddress);

    for (int curTable = 0; curTable < TABLES_DEPTH; curTable++)
    {
        physicalAddress = prevFrame * PAGE_SIZE + pageArray[curTable];
        PMread(physicalAddress, &curFrame);
        if (curFrame == 0)
        {
            // initialize parameters for the DFS function
            uint64_t addressToEvict = 0, oldCycDist = 0;
            word_t frameToEvict = 0, pageToEvict = 0, emptyFrame = 0, emptyAdd = 0;
            DFS(p, frameToEvict, pageToEvict, addressToEvict, emptyFrame, emptyAdd,
                curFrame, oldCycDist, maxFrame, used);

            // We choose the frame by traversing the entire tree of tables in the physical memory
            // while looking for one of the following (prioritized):

            if (emptyFrame)
            { // empty table
                curFrame = emptyFrame;
                PMwrite(emptyAdd, 0);
            }
            else if (maxFrame + 1 < NUM_FRAMES)
            { // unused frame
                curFrame = maxFrame + 1;
            }
            else
            { // all frames are used, evict one
                curFrame = frameToEvict;
                PMevict((uint64_t) frameToEvict, (uint64_t) pageToEvict);
                PMwrite(addressToEvict, 0);
            }

            PMwrite(physicalAddress, curFrame);
            used[curFrame] = true;
            if (curTable != TABLES_DEPTH-1)
            {
                clearTable((uint64_t) curFrame);
            }
            else
            {
                PMrestore((uint64_t) curFrame, p);
            }
        }
        prevFrame = curFrame;
    }
    return curFrame * PAGE_SIZE + calcOffset(virtualAddress);
}

/**
 * Check if the received virtual address is legal
 * @param virtualAddress - the address to check
 * @return true if legal, false otherwise
 */
bool checkFailure(uint64_t virtualAddress)
{
    return (virtualAddress >= VIRTUAL_MEMORY_SIZE);
}

/** reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value)
{
    if (checkFailure(virtualAddress))
    {
        return 0;
    }
    PMread(getPhysicalAddress(virtualAddress), value);
    return 1;
}

/** writes a word to the given virtual address
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (checkFailure(virtualAddress))
    {
        return 0;
    }
    uint64_t add = getPhysicalAddress(virtualAddress);
    PMwrite(add, value);
    return 1;
}

