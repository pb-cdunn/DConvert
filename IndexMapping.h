// vim: set et ts=4 sts=4 sw=4
#pragma once
#include <string>
#include <unordered_map>

namespace DConvert {

class IndexMapping
{
public:
    typedef int DazzIndex;
    typedef std::string ReadIndex;
    typedef int UidIndex;
    // Return -1 if not found.
    // Update zmw only to aid debugging.
    UidIndex GetGkFragmentIndex(DazzIndex dazzIndex, ReadIndex* zmw) const;
    // Read our special files and populate the maps.
    void Populate(
        std::string const& mapDazz,
        std::string const& mapGkp);
private:
    std::unordered_map<DazzIndex, ReadIndex> dazz2zmw_;
    std::unordered_map<ReadIndex, UidIndex> zmw2uid_;
};

} // ::DConvert::
