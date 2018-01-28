// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://wsic_dockosettacommons.org. Questions about this casic_dock
// (c) addressed to University of Waprotocolsgton UW TechTransfer, email: license@u.washington.eprotocols


#include <riflib/types.hh>
#include <riflib/scaffold/MorphingScaffoldProvider.hh>
#include <riflib/scaffold/util.hh>
#include <ObjexxFCL/format.hh>
#include <riflib/HSearchConstraints.hh>

#include <string>
#include <vector>
#include <boost/any.hpp>
#include <boost/format.hpp>

using ::scheme::scaffold::BOGUS_INDEX;
using ::scheme::scaffold::TreeIndex;
using ::scheme::scaffold::TreeLimits;


namespace devel {
namespace scheme {


MorphingScaffoldProvider::MorphingScaffoldProvider( 
    uint64_t iscaff,
    shared_ptr< RotamerIndex > rot_index_p_in, 
    RifDockOpt const & opt_in,
    MakeTwobodyOpts const & make2bopts_in,
    ::devel::scheme::RotamerRFTablesManager & rotrf_table_manager_in ) :

    rot_index_p( rot_index_p_in), 
    opt(opt_in),
    make2bopts(make2bopts_in),
    rotrf_table_manager(rotrf_table_manager_in) {

#ifdef USEHDF5

#else
    utility_exit_with_message("You must run cmake with the -DUSEHDF5=1 flag to use fragment insertions!!!");
#endif


    std::string scafftag;
    core::pose::Pose scaffold;
    utility::vector1<core::Size> scaffold_res;
    EigenXform scaffold_perturb;
    std::vector<CstBaseOP> csts;

    get_info_for_iscaff( iscaff, opt, scafftag, scaffold, scaffold_res, scaffold_perturb, csts);

    ScaffoldDataCacheOP temp_data_cache_ = make_shared<ScaffoldDataCache>(
        scaffold,
        scaffold_res,
        scafftag,
        scaffold_perturb,
        rot_index_p,
        opt,
        csts);

    ParametricSceneConformationCOP conformation = make_conformation_from_data_cache(temp_data_cache_, false);

    MorphMember mmember;
    mmember.conformation = conformation;

    mmember.tree_relation.depth = 0;
    mmember.tree_relation.parent_member = BOGUS_INDEX;
    mmember.tree_relation.first_child = BOGUS_INDEX;
    mmember.tree_relation.last_child = BOGUS_INDEX;


    add_morph_member( mmember );

}

void
MorphingScaffoldProvider::test_make_children(TreeIndex ti) {
    using ObjexxFCL::format::I;

#ifdef USEHDF5
    protocols::indexed_structure_store::movers::DirectSegmentLookupMover dsl_mover;

    uint64_t removed_length = opt.high_cut_site - opt.low_cut_site - 1;

    protocols::indexed_structure_store::DirectSegmentLookupConfig config;
    config.rmsd_tolerance = 0.3;
    config.segment_cluster_tolerance = opt.fragment_cluster_tolerance;
    config.max_insertion_length = removed_length + opt.max_insertion;
    dsl_mover.lookup_config( config );

    dsl_mover.from_chain( 1 );
    dsl_mover.to_chain( 2 );

    ScaffoldDataCacheOP data_cache = get_data_cache_slow( ti );

    core::pose::PoseCOP scaffold = data_cache->scaffold_full_centered_p;


    std::vector<core::pose::PoseOP> poses = apply_direct_segment_lookup_mover( 
            dsl_mover, 
            *scaffold, 
            opt.low_cut_site,
            opt.high_cut_site,
            removed_length + opt.max_insertion,
            opt.max_structures,
            opt.fragment_max_rmsd );

        std::string scafftag;
    core::pose::Pose _scaffold;
    utility::vector1<core::Size> scaffold_res;
    EigenXform scaffold_perturb;
    std::vector<CstBaseOP> csts;

    get_info_for_iscaff( 0, opt, scafftag, _scaffold, scaffold_res, scaffold_perturb, csts);

    int count = 0;
    for ( core::pose::PoseOP pose : poses ) {
        count++;

        ScaffoldDataCacheOP temp_data_cache_ = make_shared<ScaffoldDataCache>(
            *pose,
            scaffold_res,
            scafftag + boost::str(boost::format("%03i") % count),
            scaffold_perturb,
            rot_index_p,
            opt,
            csts);

        temp_data_cache_->scaffold_center += data_cache->scaffold_center;

        if ( opt.use_parent_body_energies ) {
            std::cout << "use_parent_body_energies: Preparing parent body energies" << std::endl;
            if ( ! data_cache->local_onebody_p ) {
                data_cache->setup_onebody_tables( rot_index_p, opt );
            }
            if ( ! data_cache->local_twobody_p ) {
                data_cache->setup_twobody_tables( rot_index_p, opt, make2bopts, rotrf_table_manager);
            }
            // one-body
            temp_data_cache_->scaffold_onebody_glob0_p = data_cache->scaffold_onebody_glob0_p;
            temp_data_cache_->local_onebody_p = data_cache->local_onebody_p;
            // two-body
            temp_data_cache_->scaffold_twobody_p = data_cache->scaffold_twobody_p;
            temp_data_cache_->local_twobody_p = data_cache->local_twobody_p;
        }

        ParametricSceneConformationCOP conformation = make_conformation_from_data_cache(temp_data_cache_, false);

        MorphMember mmember;
        mmember.conformation = conformation;

        mmember.tree_relation.depth = 1;
        mmember.tree_relation.parent_member = 0;
        mmember.tree_relation.first_child = BOGUS_INDEX;
        mmember.tree_relation.last_child = BOGUS_INDEX;

        pose->dump_pdb(temp_data_cache_->scafftag + ".pdb");

        add_morph_member( mmember );
    }

    if ( opt.include_parent ) {
        MorphMember const & parent_mm = get_morph_member( ti );
        
        MorphMember mmember;
        mmember.conformation = parent_mm.conformation;
        mmember.tree_relation.depth = 1;
        mmember.tree_relation.parent_member = 0;
        mmember.tree_relation.first_child = BOGUS_INDEX;
        mmember.tree_relation.last_child = BOGUS_INDEX;

        add_morph_member( mmember );
    }
#else
    utility_exit_with_message("You must run cmake with the -DUSEHDF5=1 flag to use fragment insertions!!!");
#endif



}




TreeIndex
MorphingScaffoldProvider::add_morph_member( MorphMember mmember ) {
    uint64_t depth = mmember.tree_relation.depth;
    assert( depth <= map_.size() );
    if ( depth == map_.size() ) {
        map_.resize( map_.size() + 1 );
    }
    map_[depth].push_back( mmember );
    return TreeIndex(depth, map_[depth].size());
}


ScaffoldDataCacheOP 
MorphingScaffoldProvider::get_data_cache_slow(TreeIndex i) {
    return get_scaffold( i )->cache_data_;
}

ParametricSceneConformationCOP 
MorphingScaffoldProvider::get_scaffold(TreeIndex i) {
    MorphMember & mmember = get_morph_member( i );

    return mmember.conformation;
}

MorphMember & 
MorphingScaffoldProvider::get_morph_member(TreeIndex i) {
    assert( is_valid_index( i ) );

    return map_[ i.depth ][ i.member ];
}



TreeLimits
MorphingScaffoldProvider::get_scaffold_index_limits() const {
    TreeLimits limits(map_.size());

    for ( uint64_t i = 0; i < map_.size(); i++ ) {
        limits[i] = map_[i].size();
    }
    return limits;
}


::scheme::scaffold::TreeIndex 
MorphingScaffoldProvider::get_representative_scaffold_index() {
    return TreeIndex(0, 0);
}




void 
MorphingScaffoldProvider::set_fa_mode( bool fa ) {
    for ( uint64_t i = 0; i < map_[1].size(); i++) {
        MorphMember & mmember = get_morph_member( TreeIndex(1, i) );
        ScaffoldDataCacheOP cache = mmember.conformation->cache_data_;
        if ( cache->conformation_is_fa != fa ) {
            mmember.conformation = make_conformation_from_data_cache(cache, fa);
        }
    }

}


void 
MorphingScaffoldProvider::setup_twobody_tables( ::scheme::scaffold::TreeIndex i ) {
    get_data_cache_slow( i )->setup_twobody_tables( rot_index_p, opt, make2bopts, rotrf_table_manager);
}




}}

