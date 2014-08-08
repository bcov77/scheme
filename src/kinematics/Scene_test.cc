#include <gtest/gtest.h>

#include "kinematics/Scene_io.hh"
#include "actor/ActorConcept_io.hh"
#include "numeric/X1dim.hh"
#include "objective/ObjectiveVisitor.hh"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>


#include <stdio.h>
#include <stdlib.h>

namespace scheme { namespace kinematics { namespace test {

using std::cout;
using std::endl;
using boost::tie;
using numeric::X1dim;


template<class _Interaction>
struct SetVisitor{
	typedef _Interaction Interaction;
	std::set<Interaction> set_;
	void operator()(Interaction const & i, double =1.0){
		set_.insert(i);
	}
};
template<class _Interaction>
struct AsymSetVisitor : SetVisitor<_Interaction> {
	typedef m::false_ Symmetric;
};

typedef actor::ActorConcept<X1dim,int> ADI;
typedef actor::ActorConcept<X1dim,char> ADC;	

struct FixedActor {
	double data_;
	FixedActor(double d=0) : data_(d) {}
	bool operator==(FixedActor const & o) const { return o.data_==data_; }
	bool operator<(FixedActor const & o) const { return data_ < o.data_; }

};
std::ostream & operator<<(std::ostream & out,FixedActor const & f){
	return out << "FixedActor("<<f.data_<<")";
}

template<class Interaction,class Scene>
void test_iterator_visitor_agree_for_interaction(Scene const & scene){
	typedef typename impl::get_placeholder_type<Interaction,typename Scene::Index>::type PH;
	std::set<Interaction> iterset;
	BOOST_FOREACH( PH ph, scene.template get_interactions<Interaction>() )
		iterset.insert( scene.template get_interaction<Interaction>(ph) );
	AsymSetVisitor<Interaction> v; scene.visit(v); // is asym only for 1B...
	ASSERT_EQ( iterset, v.set_ );
}

void test_iterator_visitor_agree(std::vector<int> nbod, int nsym){
	typedef m::vector< ADI, ADC, FixedActor > Actors;
	typedef Scene<Actors,X1dim,size_t> Scene;
	typedef size_t Index;
	typedef std::pair<size_t,size_t> Index2;
	typedef std::pair<Index2,Index2> Index4;

	Scene scene;
	for(int i = 1; i < nsym; ++i) scene.add_symframe(100*i);
	for(int i = 0; i < (int)nbod.size(); ++i){
		scene.add_body();
		for(int j = 0; j < nbod[i]; ++j){
			scene.mutable_conformation_asym(i).get<ADI>().push_back(ADI(i,j));
			if( j <= i )
				scene.mutable_conformation_asym(i).get<ADC>().push_back(ADC(i,(char)j));
		}
	}
	scene.mutable_conformation_asym(0).add_actor(FixedActor(1));


	{ // Asym ADI
		std::set<ADI> iterset; BOOST_FOREACH( ADI a, scene.get_actors_asym<ADI>() ) iterset.insert(a);
		AsymSetVisitor<ADI> v; scene.visit(v);
		ASSERT_EQ( iterset, v.set_ );
	}
	{ // SYM ADI
		std::set<ADI> iterset; BOOST_FOREACH( ADI a, scene.get_actors<ADI>() ) iterset.insert(a);
		SetVisitor<ADI> v; scene.visit(v);
		ASSERT_EQ( iterset, v.set_ );
	}
	{ // Asym ADC
		std::set<ADC> iterset; BOOST_FOREACH( ADC a, scene.get_actors_asym<ADC>() ) iterset.insert(a);
		AsymSetVisitor<ADC> v; scene.visit(v);
		ASSERT_EQ( iterset, v.set_ );
	}
	{ // SYM ADC
		std::set<ADC> iterset; BOOST_FOREACH( ADC a, scene.get_actors<ADC>() ) iterset.insert(a);
		SetVisitor<ADC> v; scene.visit(v);
		ASSERT_EQ( iterset, v.set_ );
	}
	{ // FixedActor
		std::set<FixedActor> iterset; BOOST_FOREACH( FixedActor a, scene.get_actors<FixedActor>() ) iterset.insert(a);
		SetVisitor<FixedActor> v; scene.visit(v);
		ASSERT_EQ( iterset, v.set_ );
	}

	test_iterator_visitor_agree_for_interaction<ADI>(scene);
	test_iterator_visitor_agree_for_interaction<ADC>(scene);
	test_iterator_visitor_agree_for_interaction<FixedActor>(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADI,ADI> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADI,ADC> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADC,ADC> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADC,ADI> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<FixedActor,ADI> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADI,FixedActor> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<FixedActor,ADC> >(scene);
	test_iterator_visitor_agree_for_interaction<std::pair<ADC,FixedActor> >(scene);

}

TEST(Scene,test_visit_against_iteration){

	test_iterator_visitor_agree(std::vector<int>(3,2),2);
	test_iterator_visitor_agree(std::vector<int>(3,4),2);
	test_iterator_visitor_agree(std::vector<int>(13,4),2);
	test_iterator_visitor_agree(std::vector<int>(2,21),7);
	test_iterator_visitor_agree(std::vector<int>(13,11),1);

}


struct Config {};

struct ObjADIFixed {
	typedef std::pair<ADI,FixedActor> Interaction;
	template<class Config>
	double operator()(Interaction const & i, Config const& ) const {
		return i.first.position().val_ * i.second.data_;
	}
};
struct ObjFixedADI {
	typedef std::pair<FixedActor,ADI> Interaction;
	template<class Config>
	double operator()(Interaction const & i, Config const& ) const {
		return i.second.position().val_ * i.first.data_;
	}
};
struct ObjFixedFixed {
	typedef std::pair<FixedActor,FixedActor> Interaction;
	template<class Config>
	double operator()(Interaction const &, Config const& ) const {
		std::exit(-1);
	}
};

TEST(Scene,test_fixed_actor){
	typedef std::pair<ADI,FixedActor> I;
	Config c;
	ObjFixedADI o1;
	ObjADIFixed o2;
	objective::ObjectiveVisitor<ObjFixedADI,Config> visitor1(o1,c);
	objective::ObjectiveVisitor<ObjADIFixed,Config> visitor2(o2,c);

	typedef m::vector< ADI, FixedActor > Actors;
	typedef Scene<Actors,X1dim,uint32_t> Scene;

	Scene scene(2);
	scene.mutable_conformation_asym(0).add_actor(FixedActor(1.0));
	scene.mutable_conformation_asym(1).add_actor(ADI(1,0));	

	scene.set_position(0,X1dim( 0));
	scene.set_position(1,X1dim( 0));
	scene.set_position(1,X1dim(-1)); scene.visit(visitor1); ASSERT_EQ( visitor1.result_ , 0.0 ); visitor1.clear();
	scene.set_position(0,X1dim(-1)); scene.visit(visitor1); ASSERT_EQ( visitor1.result_ , 1.0 ); visitor1.clear();
	scene.set_position(0,X1dim(-3)); scene.visit(visitor1); ASSERT_EQ( visitor1.result_ , 3.0 ); visitor1.clear();
	scene.set_position(1,X1dim(-5)); scene.visit(visitor1); ASSERT_EQ( visitor1.result_ ,-1.0 ); visitor1.clear();

	// cout << "========================================" << endl;

	scene.set_position(0,X1dim( 0));
	scene.set_position(1,X1dim( 0));
	scene.set_position(1,X1dim(-1)); scene.visit(visitor2); ASSERT_EQ( visitor2.result_ , 0.0 ); visitor2.clear();
	scene.set_position(0,X1dim(-1)); scene.visit(visitor2); ASSERT_EQ( visitor2.result_ , 1.0 ); visitor2.clear();
	scene.set_position(0,X1dim(-3)); scene.visit(visitor2); ASSERT_EQ( visitor2.result_ , 3.0 ); visitor2.clear();
	scene.set_position(1,X1dim(-5)); scene.visit(visitor2); ASSERT_EQ( visitor2.result_ ,-1.0 ); visitor2.clear();

	// // this should fail to compile
	// objective::ObjectiveVisitor<ObjFixedFixed,Config> failvisitor(ObjFixedFixed(),c);	
	// scene.visit(failvisitor);

}


}
}
}


















