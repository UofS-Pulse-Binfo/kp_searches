<?php

/**
 * Provides a search for SO:genetic_marker chado feature-based Tripal Content.
 *
 */
class GeneticMarkerSearch extends ChadoCustomSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Genetic Marker Search';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'genetic-markers';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = 'Search for genetic markers in our focus crops.';

  /**
   * The machine names of the permissions with access to this search. This is
   * used to map your search to existing permissions. It must be an array and
   * is used in hook_menu(). Some examples include 'access content'
   * and 'administer tripal'.
   */
  public static $permissions = ['access content'];

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'search/markers',
    // @todo support menu items.
  ];

  /**
   * Information regarding the fields and filters for this search.
   */
  public static $info = [
    // Lists the columns in your results table.
    'fields' => [
      'name' => 'Name',
      'type' => 'Marker Type',
      'species' => 'Source Species',
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'species' => [
        'title' => 'Source Species',
        'help' => 'The legume species the marker was developed for (e.g. Lens culinaris).',
      ],
      'type' => [
        'title' => 'Marker Type',
        'help' => 'The type of marker you are interested in (e.g. Illumina Golden Gate).',
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name of the marker (e.g. LcC00002p390; partial names are accepted).',
      ],
      'uniquename' => [
        'title' => 'Unique Identifier',
        'help' => 'The unique identifier of the marker (e.g. LcC00002p390; partial names are accepted).',
      ],
    ],
  ];

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Search';

  /**
   * Determine the query for the genetic marker search.
   *
   * @param string $query
   *   The full SQL query to execute. This will be executed using chado_query()
   *   so use curly brackets appropriately. Use :placeholders for any values.
   * @param array $args
   *   An array of arguments to pass to chado_query(). Keys must be the
   *   placeholders in the query and values should be what you want them set to.
   */
  public function getQuery(&$query, &$args) {

    // Retrieve the filter results already set.
    $filter_results = $this->values;
    $form_submitted = $this->submitted;

  }
}
