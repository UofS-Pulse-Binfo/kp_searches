<?php
/**
 * Provides a search specific to breeding crosses.
 */
class BreedingCrossSearch extends GermplasmSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Breeding Cross Search';

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
  public static $permissions = ['view bio_data_21'];

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
    'path' => 'search/germplasm/crosses',
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
      'crossingblock' => [
        'title' => 'Crossing Block',
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
      'crossingblock_year' => [
        'title' => 'Crossing Block Year',
        'help' => 'The year of the crossing block you are interested in.',
      ],
      'crossingblock_season' => [
        'title' => 'Crossing Block Season',
        'help' => 'The season of the crossing block you are interested in.',
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

    // Add a Maternal/paternal name selector.
    $this->addparentageFormElement($form, $form_state);

    // Make the crossing block filter more intuitive.
    $description = 'The block of crosses you are interested in. Specifically, this is the year/season that the seed from the original cross was generated in.';
    $form['crossingblock'] = array(
      '#type' => 'markup',
      '#prefix' => '<div class="crossingblock-element form-item"><label>Crossing Block</label>',
      '#suffix' => '<div class="description">' . $description . '</div></div>',
      '#weight' => -7,
    );

    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])) {
      $cbyear_default = $form_state['values']['crossingblock_year'];
      $cbseason_default = $form_state['values']['crossingblock_season'];
    }
    elseif (isset($q['crossingblock_year'])) {
      $cbyear_default = $q['crossingblock_year'];
      $cbseason_default = $q['crossingblock_season'];
    }
    else {
      $cbyear_default = NULL;
      $cbseason_default = NULL;
    }

    $seasons_flat = ['Winter', 'Spring', 'Summer', 'Fall'];
    $seasons = array_combine($seasons_flat, $seasons_flat);
    $form['crossingblock']['crossingblock_season'] = $form['species'];
    $form['crossingblock']['crossingblock_season']['#type'] = 'select';
    $form['crossingblock']['crossingblock_season']['#options'] = $seasons;
    $form['crossingblock']['crossingblock_season']['#empty_option'] = '- Select Season -';
    $form['crossingblock']['crossingblock_season']['#default_value'] = $cbseason_default;
    unset($form['crossingblock_season']);

    $years = array_combine(range(date("Y"), 1996), range(date("Y"), 1996));
    $form['crossingblock']['crossingblock_year'] = $form['species'];
    $form['crossingblock']['crossingblock_year']['#type'] = 'select';
    $form['crossingblock']['crossingblock_year']['#options'] = $years;
    $form['crossingblock']['crossingblock_year']['#empty_option'] = '- Select Year -';
    $form['crossingblock']['crossingblock_year']['#default_value'] = $cbyear_default;
    unset($form['crossingblock_year']);

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

    $query = "SELECT
        s.name,
        s.uniquename,
        s.stock_id,
        cbseason.value||' '||cbyear.value as crossingblock,
        mom.name as mom,
        mom.stock_id as mom_id,
        dad.name as dad,
        dad.stock_id as dad_id
      FROM {stock} s
      LEFT JOIN {organism} o ON o.organism_id=s.organism_id
      LEFT JOIN {stockprop} cbyear ON cbyear.stock_id=s.stock_id
        AND cbyear.type_id = 3978
      LEFT JOIN {stockprop} cbseason ON cbseason.stock_id=s.stock_id
        AND cbseason.type_id = 3977
      LEFT JOIN {stock_relationship} momr ON momr.object_id=s.stock_id
        AND momr.type_id=3632
      LEFT JOIN {stock} mom ON momr.subject_id=mom.stock_id
      LEFT JOIN {stock_relationship} dadr ON dadr.object_id=s.stock_id
        AND dadr.type_id=3633
      LEFT JOIN {stock} dad ON dadr.subject_id=dad.stock_id";

    $where = [];
    $joins = [];

    // -- Restrict to Breeding Crosses.
    $where[] = 's.type_id=:type_id';
    $args[':type_id'] = 4335;

    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "o.genus ~* :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "o.species ~* :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "(s.name ~* :name OR s.uniquename = :name)";
      $args[':name'] = $filter_results['name'];
    }

    // -- Crossing Block Year.
    if ($filter_results['crossingblock_year'] != '') {
      $where[] = "cbyear.value = :cbyear";
      $args[':cbyear'] = $filter_results['crossingblock_year'];
    }

    // -- Crossing Block Season.
    if ($filter_results['crossingblock_season'] != '') {
      $where[] = "cbseason.value = :cbseason";
      $args[':cbseason'] = $filter_results['crossingblock_season'];
    }

    // -- Parents.
    $this->addParentageQuery($filter_results, $where, $args);

    // Finally, add it to the query.
    if (!empty($joins)) {
      $query .= implode("\n",$joins);
    }
    if (!empty($where)) {
      $query .= "\n" . ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= "\n" . ' ORDER BY cbyear.value DESC, cbseason.value ASC, s.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= "\n" . ' LIMIT ' . $limit . ' OFFSET ' . $offset;

    // @debug dpm(strtr(str_replace(['{','}'], ['chado.', ''], $query), $args), 'query');
  }
}
