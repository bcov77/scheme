#include <gtest/gtest.h>

#include "scheme/nest/NEST.hh"
#include "scheme/nest/maps/ScaleMap.hh"
#include "scheme/kinematics/Director.hh"
#include "scheme/numeric/X1dim.hh"

#include <boost/timer/timer.hpp>

namespace scheme { namespace kinematics { namespace test_director {

using std::cout;
using std::endl;
using numeric::X1dim;

struct TestScene : public SceneBase<X1dim>
{

	TestScene() : SceneBase<X1dim>() {}
	virtual ~TestScene(){}

	void add_position( X1dim const & x ) {
		this->positions_.push_back(x);
		update_symmetry(positions_.size());
	}

	virtual shared_ptr<SceneBase<X1dim> > clone() const { return make_shared<TestScene>(*this); }


};

std::ostream & operator<<( std::ostream & out, TestScene const & s ){
	out << "TestScene";
	BOOST_FOREACH( X1dim x, s.positions_ ) out << "\t" << x;
	return out;
}

TEST( Director, basic_test ){
	// cout << "Director" << endl;

	TestScene scene;
	scene.add_position(0);
	scene.add_position(0);	
	// cout << scene << endl;

	shared_ptr<scheme::nest::NestBase<> > x1nest  = make_shared<scheme::nest::NEST<1,X1dim> >(1);
	shared_ptr<scheme::nest::NestBase<> > x1nest2 = make_shared<scheme::nest::NEST<1,X1dim> >(2);


	shared_ptr< SceneTree< X1dim > > child = make_shared< SceneTree< X1dim > >(10);
	child->add_body(1);
	child->add_position_nest(x1nest2);

	shared_ptr< SceneTree< X1dim > > root = make_shared< SceneTree< X1dim > >(20);
	root->add_body(0);
	root->add_position_nest(x1nest);
	root->add_child(child);

	NestDirector< numeric::X1dim > director(root);

	nest::NEST<2,util::SimpleArray<2>,nest::maps::ScaleMap> refnest( 
		util::SimpleArray<2>(20.0,10.0), 
		util::SimpleArray<2>(21.0,12.0),
		util::SimpleArray<2,uint64_t>(1,2)
	 );


	boost::timer::cpu_timer t;
	int count = 0;
	for( int resl = 0; resl < 8; ++resl ){
		// cout << "================== resl " << resl << " ======================" << endl;
		for( uint64_t i = 0; i < director.size(resl); ++i){
			util::SimpleArray<2> tmp = refnest.set_and_get(i,resl);
			// scene.set_position( 0, tmp[0] );
			// scene.set_position( 1, tmp[1] );
			director.set_scene( i, resl, scene ); 
			++count;
			// cout << scene << " " << refnest.set_and_get(i,resl) << endl;
			ASSERT_EQ( scene.position(0)[0], tmp[0] );
			ASSERT_EQ( scene.position(1)[0], tmp[1] );
		}
	}
	cout << "set_scene rate: " << (double)count / t.elapsed().wall*1000000000.0 << " / sec " << endl;


	shared_ptr<SceneBase<X1dim> > test = scene.clone();
	// cout << test->position(0) << " " << scene.position(0) << endl;
	ASSERT_EQ( test->position(0)[0] , scene.position(0)[0] );	
	test->set_position(0,0);
	// cout << test->position(0) << " " << scene.position(0) << endl;	
	ASSERT_NE( test->position(0)[0] , scene.position(0)[0] );
}

}
}
}
