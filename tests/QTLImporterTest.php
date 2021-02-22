<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;
use Tests\DatabaseSeeders\GeneticMapSeeder;
use Faker\Factory;

class QTLImporterTest extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  /**
   * Test the importer with invalid arguements.
   */
  public function testRunExpectFail() {
    // Supress tripal errors
    putenv("TRIPAL_SUPPRESS_ERRORS=TRUE");
    ob_start();

    // Prep.
    $trait_term = factory('chado.cvterm')->create([
      'name' => 'Days to Flowering',
    ]);
    $seeder = GeneticMapSeeder::seed();
    $mapdetails = $seeder->getDetails();

    // -- With no map.
    $args = ['trait_cv_id' => $trait_term->cv_id ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();
    $success = $importer->run();
    $this->assertFalse($success,
      "The importer should not run with no genetic map.");

    // -- With no trait.
    $args = ['featuremap_name' => $mapdetails['featuremap_id'] ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();
    $success = $importer->run();
    $this->assertFalse($success,
      "The importer should not run with no trait.");

    // -- With an invalid trait.
    $args = [
      'featuremap_name' => $mapdetails['featuremap_id'],
      'trait_cv_id' => 999999999999999999,
    ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();
    $success = $importer->run();
    $this->assertFalse($success,
      "The importer should not run with invalid trait.");

    // -- With an invalid map.
    $args = [
      'featuremap_name' => 999999999999999999,
      'trait_cv_id' => $trait_term->cv_id,
    ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();
    $success = $importer->run();
    $this->assertFalse($success,
      "The importer should not run with an invalid map.");

    // -- With an invalid file.
    $args = [
      'featuremap_name' => 999999999999999999,
      'trait_cv_id' => $trait_term->cv_id,
    ];
    $file = 'fake/file/mcfakerson.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();
    $success = $importer->run();
    $this->assertFalse($success,
      "The importer should not run without a file.");

    // -- with a test file designed to hit
    // Clean the buffer and unset tripal errors suppression
    ob_end_clean();
    putenv("TRIPAL_SUPPRESS_ERRORS");
  }

  /**
   * Tests the core importer.
   */
  public function testRun() {
    // Supress tripal errors
    putenv("TRIPAL_SUPPRESS_ERRORS=TRUE");
    ob_start();

    $seeder = GeneticMapSeeder::seed();
    $mapdetails = $seeder->getDetails();

    $trait_term = factory('chado.cvterm')->create([
      'name' => 'Days to Flowering',
    ]);

    // Instatiate the loader.
    $args = [
      'featuremap_name' => $mapdetails['featuremap_id'],
      'trait_cv_id' => $trait_term->cv_id,
    ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();

    // Now we run the importer!
    $success = $importer->run($mapdetails['featuremap_id']);
    $this->assertNotFalse($success,
      "The importer returned an error.");

    // Finally, check that the data is in the correct tables, etc.
    // -- check that the QTL features were created.
    $qtl = chado_query("SELECT * FROM {feature} f
      WHERE f.type_id IN (SELECT cvterm_id FROM {cvterm} WHERE name='QTL')
      AND f.organism_id=:org", [':org' => $mapdetails['organism_id']])->fetchAll();
    $this->assertCount(4, $qtl, "There was not the expected number of QTL inserted (organism_id=".$mapdetails['organism_id'].").");

    // -- check the published names match.
    $names = ['LcC23363p108-DTF-SPG2011', 'LcC06044p758-DTF-Preston2009',
      'Yc-DTF-Preston2011', 'Yc-DTF-Preston2009'];
    $qtl_ids = [];
    foreach ($qtl as $q) {
      $this->assertContains($q->name, $names,
        "The QTL retrieved is not one of the names we expected.");
      $qtl_ids[] = $q->feature_id;
    }

    // -- check that each QTL is positioned on the map.
    $positions = chado_query('SELECT * FROM chado.featurepos WHERE feature_id IN (:ids)',
      [':ids' => $qtl_ids])->fetchAll();
    $this->assertCount(4, $positions, "There was not the expected number of locations for our set of QTL.");
    foreach ($positions as $pos) {
      $this->assertEquals($mapdetails['featuremap_id'], $pos->featuremap_id,
        "We have a position for the QTL but it is not on our map.");
    }

    // Clean the buffer and unset tripal errors suppression
    ob_end_clean();
    putenv("TRIPAL_SUPPRESS_ERRORS");
  }

  /**
   * Test the importer form.
   */
  public function testForm() {

    // Instatiate the loader.
    $args = [
      'featuremap_name' => $mapdetails['featuremap_id'],
      'trait_cv_id' => $trait_term->cv_id,
    ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);
    $importer->prepareFiles();

    $form_state = [];
    $form = $importer->form([], $form_state);

    $this->assertArrayHasKey('featuremap_name', $form,
      "The form doesn't contain a field for the genetic map name.");
    $this->assertArrayHasKey('trait_cv_id', $form,
      "The form doesn't contain a field for the CV containing traits.");
  }

  /**
   * Tests the helper functions for the QTL importer.
   */
  public function testHelpers() {

    // Instatiate the loader.
    $featuremap = factory('chado.featuremap')->create();
    $args = [
      'featuremap_name' => $featuremap->featuremap_id,
    ];
    $file = 'example_files/qtl.singletrait.tsv';
    $importer = $this->instantiateImporter($file, $args);

    // Values for testing.
    $faker = Factory::create();
    $organism = factory('chado.organism')->create();
    $values = [
      'name' => $faker->name(),
      'uniquename' => $faker->word() . uniqid(),
      'organism_id' => $organism->organism_id,
      'type_id' => ['name' => 'QTL', 'cv_id' => ['name' => 'sequence']],
    ];

    // TEST lookup_id(): no record.
    $retrieved_id = $importer->lookup_id('feature',$values,'feature_id');
    $this->assertFalse($retrieved_id, "Incorrectly found an id that shouldn't exist?");

    // TEST save_datapoint(): insert new.
    $id = $importer->save_datapoint('feature',$values,'feature_id');
    $this->assertNotFalse($id, "Unable to insert the feature using save_datapoint().");
    $this->assertTrue(is_numeric($id), "Feature ID returned from save_datapoint() is not numeric.");

    // TEST lookup_id(): existing record.
    $retrieved_id = $importer->lookup_id('feature',$values,'feature_id');
    $this->assertEquals($id, $retrieved_id, "Unable to lookup the newly created feature with lookup_id().");

    // TEST save_datapoint(): update existing.
    $new_id = $importer->save_datapoint('feature',$values,'feature_id');
    $this->assertNotFalse($new_id, "Unable to update the feature using save_datapoint().");
    $this->assertTrue(is_numeric($new_id), "Feature ID returned from save_datapoint() is not numeric.");
    $this->assertEquals($id, $new_id, "ID returned from save_datapoint() does not match the previously inserted one.");

    // TEST save_feature_property: new property.
    $symbol = $faker->word();
    $success = $importer->save_feature_property($id, 'MAIN', 'published_symbol', $symbol);
    $this->assertNotFalse($success, "Not able to insert property.");
    $success = $importer->save_feature_property($id, 'MAIN', 'published_symbol', $symbol);
    $this->assertNotFalse($success, "Not able to update property.");
  }

  /**
   * Instantiate the importer object for tests.
   */
  private function instantiateImporter($rel_filepath, $run_args) {
    // Load our importer into scope.
    module_load_include('inc', 'tripal_qtl', 'includes/TripalImporter/QTLImporter');

    // Create a new instance of our importer.
    $path = drupal_get_path('module', 'tripal_qtl') . '/tests/';
    $file = ['file_local' => DRUPAL_ROOT . '/' . $path . $rel_filepath];
    $importer = new \QTLImporter();
    $importer->create($run_args, $file);

    return $importer;
  }
}
