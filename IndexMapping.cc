// vim: set et ts=4 sts=4 sw=4
#include "IndexMapping.h"
#include <iostream>  // cerr
#include <fstream>
#include <exception>
using namespace DConvert;

IndexMapping::UidIndex IndexMapping::GetGkFragmentIndex(DazzIndex dazzIndex, ReadIndex* zmw) const
{
    auto dit = dazz2zmw_.find(dazzIndex);
    if (dit == dazz2zmw_.end()) return -1;
    *zmw = dit->second;
    auto zit = zmw2uid_.find(*zmw);
    if (zit == zmw2uid_.end()) return -1;
    int const gkpid = zit->second;
    if (gkpid == 0)
    {
        std::cerr << "[WARN]dazzIndex=" << dazzIndex << " => zmw=" << *zmw << " => gkpid=" << gkpid << " which is illegal.\n";
    }
    return gkpid;
}
void IndexMapping::Populate(
      std::string const& mapDazz,
      std::string const& mapGkp)
{
    // sprintf(fastqUIDmapName, "%s.fastqUIDmap", gkp_name.c_str());
    std::cerr << "name:'" << mapGkp << "'\n";
    std::ifstream fni(mapGkp.c_str());
    if (!fni) throw std::runtime_error(mapGkp);
    while (fni) {
        int uid;
        ReadIndex zmw;
        fni >> uid >> zmw;
        // std::cerr << uid << "<-" << zmw << "\n";
        zmw2uid_[zmw] = uid;
    }
    std::cerr << "nuniq:" << zmw2uid_.size() << "\n";

    std::cerr << "name:'" << mapDazz << "'\n";
    std::ifstream fdazz(mapDazz.c_str());
    if (!fdazz) throw std::runtime_error(mapDazz);
    while (fdazz) {
        DazzIndex dazz_id, length;
        ReadIndex zmw;
        fdazz >> dazz_id >> zmw >> length;
        dazz2zmw_[dazz_id] = zmw;
    }
    std::cerr << "nuniq:" << dazz2zmw_.size() << "\n";
}
