<?php
/**
 * Provides a search specific to breeding crosses.
 */
class CultivarSearch extends GermplasmSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Registered Variety Search';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'kp_searches';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = '';

  /**
   * The machine names of the permissions with access to this search. This is
   * used to map your search to existing permissions. It must be an array and
   * is used in hook_menu(). Some examples include 'access content'
   * and 'administer tripal'.
   */
  public static $permissions = ['view bio_data_22'];

  /**
   * If TRUE, this search will require the submit button to be clicked before
   * executing the query; whereas, if FALSE it will be executed on the
   * first page load without parameters.
   *
   * NOTE: to control the results without parameters check $this->submitted
   * in getQuery() and if FALSE return your pre-submit query.
   */
  public static $require_submit = TRUE;

  /**
   * Add a pager to search results
   * with $num_items_per_page results on a single page.
   * NOTE: Your query has to handle paging.
   */
  public static $pager = TRUE;
  public static $num_items_per_page = 50;

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'search/germplasm/varieties',
    // @todo support menu items.
  ];

  /**
   * Add CSS/JS to the form/results page.
   * NOTE: paths supplied should be relative to $module.
   */
  public static $attached = [
    'css' => [
      'css/all_kp_searches.css',
    ],
    'js' => [],
  ];

  /**
   * Information regarding the fields and filters for this search.
   */
  public static $info = [
    // Lists the columns in your results table.
    'fields' => [
      'name' => [
        'title' => 'Name',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'stock_id'
        ],
      ],
      'mom' => [
        'title' => 'Maternal Parent',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'mom_id'
        ],
      ],
      'dad' => [
        'title' => 'Paternal Parent',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'dad_id'
        ],
      ],
      'collection' => [
        'title' => 'Collection'
      ],
      'source_institute' => [
        'title' => 'Source Institute',
      ],
      'market_class' => [
        'title' => 'Market Class',
      ],
      'yr_released' => [
        'title' => 'Year Released',
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'genus' => [
        'title' => 'Genus',
        'help' => 'The legume genus the germplasm belongs to (e.g. Lens).',
      ],
      'species' => [
        'title' => 'Species',
        'help' => 'The legume species the germplasm belongs to (e.g. culinaris).',
      ],
      'collection' => [
        'title' => 'Collection',
        'help' => 'The collection or population the germplasm is part of.'
      ],
      'source_institute' => [
        'title' => 'Source Institute',
        'help' => 'The institute the germplasm was developed or collected by.'
      ],
      'market_class' => [
        'title' => 'Market Class',
        'help' => 'Groupings from each crop based on phenotypic traits desirable by economic groups (e.g. Large Red).'
      ],
      'mom' => [
        'title' => 'Maternal Parent',
        'help' => 'The maternal parent of the germplasm you are interested in.'
      ],
      'dad' => [
        'title' => 'Paternal Parent',
        'help' => 'The paternal parent of the germplasm you are interested in.'
      ],
      'reciprocal_crosses' => [
        'title' => 'Include reciprocal crosses',
        'help' => 'Includes both A x B and B x A crosses when using the parentage filter.',
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name or accession of the germplasm (e.g. CDC Redberry; partial names are accepted).',
      ],
    ],
  ];

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Search';

  /**
   * Generate the filter form.
   *
   * The base class will generate textfields for each filter defined in $info,
   * set defaults, labels and descriptions, as well as, create the search
   * button.
   *
   * Extend this method to alter the filter form.
   */
  public function form($form, $form_state) {
    $form = ChadoCustomSearch::form($form, $form_state);

    // Add a crop selector.
    $this->addCropFormElement($form, $form_state);

    // Add a Scientific name selector.
    $this->addSpeciesFormElement($form, $form_state);

    // Add collection/population selector.
    $this->addCollectionFormElement($form, $form_state);

    // Add a Maternal/paternal name selector.
    $this->addparentageFormElement($form, $form_state);

    // Move name to the bottom.
    $form['name']['#weight'] = 10;

    return $form;
  }
  /**
   * Allows custom searches to validate the form results.
   * Use form_set_error() to signal invalid values.
   */
  public function validateForm($form, $form_state) { }

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
  public function getQuery(&$query, &$args, $offset) {
    global $user;

    // Retrieve the filter results already set.
    $filter_results = $this->values;
    // @debug dpm($filter_results, '$filter_results');

    // To get the parents...
    // 1) Cultivar -is_registered_cultivar_of-> individual -is_selection_of-> cross
    // 2) Cultivar -is_registered_cultivar_of-> cross
    // type_id: is_registered_cultivar_of = 3679; is_selection_of = 3684;
    //    source_institute = 3711
    $query = "SELECT
        s.name,
        s.uniquename,
        s.stock_id,
        source.value as source_institute,
        released.value as yr_released,
        market.value as market_class,
        mom.name as mom,
        mom.stock_id as mom_id,
        dad.name as dad,
        dad.stock_id as dad_id,
        col.name as collection
      FROM {stock} s
      LEFT JOIN {organism} o ON o.organism_id=s.organism_id
      LEFT JOIN {stockprop} source ON source.stock_id=s.stock_id AND source.type_id=3711
      LEFT JOIN {stockprop} market ON market.stock_id=s.stock_id
        AND (market.type_id=4086 OR market.type_id=3682)
      LEFT JOIN {stockprop} released ON released.stock_id=s.stock_id AND released.type_id=3688
      LEFT JOIN {stock_relationship} regcultivar ON regcultivar.subject_id=s.stock_id
        AND regcultivar.type_id = 3679
      LEFT JOIN {stock_relationship} origcross ON origcross.subject_id=regcultivar.object_id
        AND origcross.type_id = 3684
      LEFT JOIN {stock_relationship} momr
        ON (momr.object_id=origcross.object_id OR momr.object_id=regcultivar.object_id)
        AND momr.type_id=3632
      LEFT JOIN {stock} mom ON momr.subject_id=mom.stock_id
      LEFT JOIN {stock_relationship} dadr
        ON (dadr.object_id=origcross.object_id OR dadr.object_id=regcultivar.object_id)
        AND dadr.type_id=3633
      LEFT JOIN {stock} dad ON dadr.subject_id=dad.stock_id
      LEFT JOIN {stockcollection_stock} c ON s.stock_id = c.stock_id 
      LEFT JOIN {stockcollection} col ON c.stockcollection_id = col.stockcollection_id";

    $where = [];
    $joins = [];

    // -- Restrict to Breeding Crosses.
    $where[] = 's.type_id=:type_id';
    $args[':type_id'] = 4991;

    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "o.genus ~* :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "o.species ~ :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Collection: 0 - select all.
    if ($filter_results['collection'] > 0) {
      $where[] = "col.stockcollection_id = :collection";
      $args[':collection'] = $filter_results['collection'];
    }

    // -- Source Institute.
    if ($filter_results['source_institute'] != '') {
      $where[] = "source.value ~* :source_institute";
      $args[':source_institute'] = $filter_results['source_institute'];
    }

    // -- Market Class
    if ($filter_results['market_class'] != '') {
      $where[] = "market.value ~* :market_class";
      $args[':market_class'] = $filter_results['market_class'];
    }

    // -- Parents.
    $this->addParentageQuery($filter_results, $where, $args);

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "(s.name ~* :name OR s.uniquename = :name)";
      $args[':name'] = $filter_results['name'];
    }

    // Finally, add it to the query.
    if (!empty($joins)) {
      $query .= implode("\n",$joins);
    }
    if (!empty($where)) {
      $query .= "\n" . ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= "\n" . ' ORDER BY s.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= "\n" . ' LIMIT ' . $limit . ' OFFSET ' . $offset;

    // @debug dpm(strtr(str_replace(['{','}'], ['chado.', ''], $query), $args), 'query');
  }
}
